CC = gcc
shell: shell.c
	$(CC) $< -o $@

.PHONY: clean

clean:
	rm shell
