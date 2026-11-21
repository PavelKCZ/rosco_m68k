# IMPLEMENTED / NOT EXECUTED. Requires separate T2-V1 authorization.
# No compiler is queried by Make expansion; no $(shell ...) is used.
.DEFAULT_GOAL := help
.NOTPARALLEL:
PYTHON := python3
export PYTHONDONTWRITEBYTECODE := 1
PROFILE := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
RUNTIME := $(abspath $(PROFILE)/..)
DRIVER := $(PROFILE)/tools/build.py
PROGRAM := m31-sdfat-smoke-diag
# Exact CPU/C/link flags and the explicit source table live in tools/profile.py.
# build.py executes and records that table; no original Makefile is included.
NAMES := startup.o memory.o runtime.o syscalls.o termios.o fat_ro.o sd_ro.o csd_decode.o sd_ro_traps.o b3_integration.o application.o machine.o
.PHONY: help objects link binary inspect audit negative-audit validate
help:
	@echo 'IMPLEMENTATION ONLY; no validation has run. After authorization use BUILD=/tmp/new-empty-directory.'
	@echo 'Targets: objects, link, binary, inspect, audit, negative-audit, validate.'
# Phony actions intentionally recheck all pinned inputs and rebuild when invoked;
# header changes can never silently reuse stale objects. All outputs stay in BUILD.
objects:
	$(PYTHON) $(DRIVER) --build "$(BUILD)" compile
link:
	$(PYTHON) $(DRIVER) --build "$(BUILD)" link
binary:
	$(PYTHON) $(DRIVER) --build "$(BUILD)" binary
inspect:
	$(PYTHON) $(DRIVER) --build "$(BUILD)" inspect
audit:
	$(PYTHON) $(DRIVER) --build "$(BUILD)" audit
negative-audit:
	$(PYTHON) $(DRIVER) --build "$(BUILD)" negative
validate: audit negative-audit
	@echo 'Machine audit and negative cases completed. This is NOT hardware authorization.'
