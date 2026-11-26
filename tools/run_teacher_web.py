"""Run from repo root: python -m tools.run_teacher_web [--test] [--bootstrap]."""
import argparse
import getpass
import logging
import os
from pathlib import Path
from wsgiref.simple_server import make_server, WSGIRequestHandler

from tools.teacher_web import TeacherWeb, ORIGIN

LOG = logging.getLogger(__name__)


class LocalHandler(WSGIRequestHandler):
    # Single-threaded host keeps the existing process-local session store safe.
    def setup(self):
        super().setup()
        self.connection.settimeout(10)

    def log_message(self, fmt, *args):
        # Do not record arbitrary paths, queries or request headers.
        pass


def main():
    try:
        logging.basicConfig(level=logging.INFO)
        parser = argparse.ArgumentParser()
        parser.add_argument("--bootstrap", action="store_true")
        parser.add_argument("--test", action="store_true", help="Isolated local test data; 4-character minimum")
        args = parser.parse_args()
        directory = Path(os.environ.get("LOCALAPPDATA", str(Path.home()))) / "PocketLearningStudio"
        if args.test:
            directory = directory.with_name("PocketLearningStudioTest")
            LOG.warning("TEST MODE: synthetic data only; separate account and deck databases")
        app = TeacherWeb(directory, test_mode=args.test)
        if args.bootstrap:
            user = input("First administrator username: ")
            password = getpass.getpass(f"Password ({app.accounts.minimum_password_length}+ characters): ")
            if password != getpass.getpass("Confirm password: "):
                raise ValueError("Passwords do not match")
            app.accounts.create(user, password, "admin", bootstrap=True)
            LOG.info("Initial administrator created")
            return
        with make_server("127.0.0.1", 8765, app, handler_class=LocalHandler) as server:
            LOG.info("Open %s in your browser. Ctrl+C stops the server.", ORIGIN)
            server.serve_forever()
    except KeyboardInterrupt:
        LOG.info("Teacher Studio stopped")
    except Exception:
        LOG.exception("Teacher Studio startup failed")
        raise SystemExit(1)


if __name__ == "__main__":
    main()
