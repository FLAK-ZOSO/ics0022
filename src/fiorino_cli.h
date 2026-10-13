#pragma once

int fiorino_cli_signo(void);
int fiorino_cli_raised(void);

int fiorino_cli_install_signal_handler(void (*callback)(int));
