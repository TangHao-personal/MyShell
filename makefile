CC = gcc
Warn = -Wall

shell: shell.c
	$(CC) $(Warn) $< -o $@

.PHONY: clean

clean:
	rm shell
