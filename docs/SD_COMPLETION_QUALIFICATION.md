# Physical SD completion qualification

Adult development Tab5 only. No child data. The normal learner preview must not
enable persistent SD completion until these checks succeed. Use a spare, independently
backed-up compatible card; the flash backup does not include removable SD contents.

1. Verify ordinary learner boots and close the serial monitor. Record the current
   firmware revision and preserve the verified recovery folder.
2. Run tools/start_sd_write_test.ps1 with full checked-out revision, exact MAC and
   COM port. The installer builds the isolated sd_write_probe and checks the device,
   two full flash backups and live partition layout before explicit installation.
3. Record the first screen: WRITE AND REOPEN PASSED: 192 bytes. If the fixture
   already exists, a read-only pass alone cannot prove a new write. Use a separate
   clean test card, not automatic deletion or overwrite.
4. Restart normally; record EXISTING FIXTURE VERIFIED (read only). Record capacity
   and any error. Remove the card only after shutdown. On a PC, confirm the file
   PLTST01.BIN is exactly 192 bytes and unrelated backed-up files are unchanged.
5. Restore the original learner using tools/restore_tab5_application.ps1 with
   -RecoveryFolder, -ExpectedMac and -Port. First run without -Install. Then run
   with -Install and explicitly confirm RESTORE APP. The script restores the exact
   original application slot and verifies full readback; no NVS, bootloader,
   partition table or SD writes. Confirm ordinary learner boot.
6. Only after the isolated write/restart and restore are physically accepted,
   consider a separately authorized learner_sd_eval build using synthetic data and
   the fixed PLEVAL01.BIN path. Test one completion, restart, finished-list review,
   no duplicate completion, resume preservation and unavailable-card behavior.
   Do not promote the evaluation build to regular learner firmware by default.

Record evidence for each step. Any uncertain write or corrupted fixture must
stop the test and preserve the affected file. No formatting, repair or retries.
