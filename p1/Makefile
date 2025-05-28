# CMSC216 Project 1 Makefile
# Updated: Wed Feb 12 09:05:03 PM EST 2025
AN = p1
CLASS = 216

# -Wno-comment: disable warnings for multi-line comments, present in some tests
# -Werror=format-security: warn/error for using printf() with raw strings
CFLAGS = -Wall -g -Wno-unused-variable
CC     = gcc $(CFLAGS)
SHELL  = /bin/bash
.SHELLFLAGS = -O nullglob -c
CWD    = $(shell pwd | sed 's/.*\///g')

PROGRAMS = \
	mazesolve_main         \
	test_mazesolve_funcs

export PARALLEL?=True		#enable parallel testing if not overridden

all : $(PROGRAMS) 

# cleaning target to remove compiled programs/objects
clean :
	rm -f $(PROGRAMS) *.o vgcore.*

help :
	@echo 'Typical usage is:'
	@echo '  > make                          # build all programs'
	@echo '  > make clean                    # remove all compiled items'
	@echo '  > make zip                      # create a zip file for submission'
	@echo '  > make prob1                    # built targets associated with problem 1'
	@echo '  > make test                     # run all tests'
	@echo '  > make test-prob2               # run test for problem 2'
	@echo '  > make test-prob2 testnum=5     # run problem 2 test #5 only'
	@echo '  > make update                   # download and install any updates to project files'


############################################################
# 'make zip' to create complete.zip for submission
ZIPNAME = $(AN)-complete.zip
zip : clean clean-tests
	rm -f $(ZIPNAME)
	cd .. && zip "$(CWD)/$(ZIPNAME)" -r "$(CWD)"
	@echo Zip created in $(ZIPNAME)
	@if (( $$(stat -c '%s' $(ZIPNAME)) > 10*(2**20) )); then echo "WARNING: $(ZIPNAME) seems REALLY big, check there are no abnormally large test files"; du -h $(ZIPNAME); fi
	@if (( $$(unzip -t $(ZIPNAME) | wc -l) > 256 )); then echo "WARNING: $(ZIPNAME) has 256 or more files in it which may cause submission problems"; fi

############################################################
# `make update` to get project updates
update :
ifeq ($(findstring solution,$(CWD)),)
	curl -s https://www.cs.umd.edu/~profk/216/$(AN)-update.sh | /bin/bash 
else
	@echo "Cowardly refusal to update solution"
endif

############################################################
# maze solving problem
mazesolve_main : mazesolve_main.o mazesolve_funcs.o 
	$(CC) -o $@ $^

mazesolve_main.o : mazesolve_main.c mazesolve.h
	$(CC) -c $<

mazesolve_funcs.o : mazesolve_funcs.c mazesolve.h
	$(CC) -c $<

test_mazesolve_funcs : test_mazesolve_funcs.c mazesolve_funcs.o 
	$(CC) -o $@ $^

# problem targets
prob1 : mazesolve_funcs.o test_mazesolve_funcs

prob2 : mazesolve_funcs.o test_mazesolve_funcs

prob3 : mazesolve_funcs.o test_mazesolve_funcs

prob4 : mazesolve_main test_mazesolve_funcs

# Testing Targets
test : test-prob1 test-prob2 test-prob3 test-prob4

test-setup:
	@chmod u+x testy

test-prob1 : test_mazesolve_funcs test-setup
	./testy -o md test_mazesolve1.org $(testnum)

test-prob2 : test_mazesolve_funcs test-setup
	./testy -o md test_mazesolve2.org $(testnum)

test-prob3 : test_mazesolve_funcs mazesolve_main test-setup
	./testy -o md test_mazesolve3.org $(testnum)

test-prob4 : test_mazesolve_funcs mazesolve_main test-setup
	./testy -o md test_mazesolve4.org $(testnum)

test-makeup : mazesolve_main test-setup
	./testy -o md test_mazesolve_makeup.org $(testnum)

clean-tests :
	rm -rf test-results


