"""Teacher Studio synthetic student recovery, with scoped identity at every action."""
import hashlib
import hmac
import logging
import re

from tools.archive_envelope import new_recovery_key
from tools.protected_math_files import ProtectedMathFiles
from tools.protected_backup_center import HEX
from tools.student_backup_views import student_page

LOG = logging.getLogger(__name__)
NAME = re.compile(r"pocket-student-test-[a-f0-9]{32}\.plse\Z")


class StudentBackupCenter:
    def __init__(self, archive, folder_service):
        self.files = ProtectedMathFiles(archive)
        self.registry = archive.registry
        self.folders = folder_service

    def _identity(self, token, student):
        # Derive device from authorized registry; never trust posted device IDs.
        for row in self.registry.list_assignments(token):
            if row["student_id"] == student and row["status"] == "PLANNED":
                return student, row["device_id"]
        raise PermissionError("Student assignment unavailable")

    @staticmethod
    def _proof(key, token, action, student, device, *parts):
        data = "\0".join(("student-backup-v1", token, action, student, device, *parts))
        return hmac.new(key, data.encode(), hashlib.sha256).hexdigest()

    def page(self, token, csrf, *, student=None, **kwargs):
        assignments = self.registry.list_assignments(token)
        selected = self._identity(token, student) if student is not None else None
        _, folder = self.folders._folder(token)
        return student_page(assignments, csrf, folder=folder, selected=selected,
                            name_pattern=NAME, **kwargs)

    def handle(self, token, csrf, key, data):
        student = data.get("student")
        try:
            student, device = self._identity(token, student)
            action = data.get("action")
            common = {"csrf","action","student"}
            fields = set(data)
            def proof(purpose, *parts):
                return self._proof(key, token, purpose, student, device, *parts)
            def check(expected):
                supplied = data.get("proof", "")
                if not HEX.fullmatch(supplied) or not hmac.compare_digest(expected, supplied):
                    raise PermissionError("Student backup review required")
            def page(**kwargs):
                return self.page(token, csrf, student=student, **kwargs)
            if action == "select" and fields == common:
                return page()
            if action == "generate" and fields == common:
                value = new_recovery_key()
                return page(generated=value, proof=proof("generate", value))
            _, folder = self.folders._folder(token)
            if action == "save" and fields == common | {"proof","recovery_key","repeat_key","saved_key"}:
                value = data["recovery_key"]
                if not HEX.fullmatch(value) or not HEX.fullmatch(data["repeat_key"]) or data["saved_key"] != "yes" or not hmac.compare_digest(value, data["repeat_key"]):
                    raise ValueError("Retain and confirm key")
                check(proof("generate",value))
                receipt = self.files.save(token, device, student, folder, recovery_key=value)
                return page(notice=f"Verified mathematics archive saved: {receipt.path.name}; {receipt.records} attempts.")
            if action in {"preview","restore"}:
                name = data.get("filename","")
                if not NAME.fullmatch(name):
                    raise ValueError("Invalid archive selection")
                path = folder / name
                if not path.is_file() or path.is_symlink():
                    raise ValueError("Archive unavailable")
                if action == "preview" and fields == common | {"filename","recovery_key"}:
                    summary = self.files.preview(token,device,student,path,recovery_key=data["recovery_key"])
                    return page(filename=name,digest=summary.sha256,summary=summary,
                                proof=proof("restore",name,summary.sha256))
                if action == "restore" and fields == common | {"filename","digest","proof","recovery_key"}:
                    digest = data["digest"]
                    if not HEX.fullmatch(digest):
                        raise ValueError("Invalid archive digest")
                    check(proof("restore",name,digest))
                    self.files.restore(token,device,student,path,recovery_key=data["recovery_key"],
                                       expected_sha256=digest,confirmed=True)
                    return page(notice="Student mathematics archive restored and verified on this computer.")
            raise ValueError("Invalid student backup operation")
        except PermissionError:
            raise
        except (ValueError,OSError,ImportError):
            LOG.warning("Student backup or restore failed")
            return self.page(token,csrf,student=student,notice="Backup or restore failed. Check saved history, key and archive.")
