# Host-side driver: builds the container image on demand and runs the intro
# build inside it. Artifacts land in the repository's obj/ and out/, exactly
# where the Windows .bat files put them.
#
#   make -C linux-cross fast     # release build, seconds       -> out/intro-fast.exe
#   make -C linux-cross slow     # release build, minutes       -> out/intro-slow.exe
#   make -C linux-cross debug    # windowed, no music, GLCHECK  -> out/intro-debug.exe
#   make -C linux-cross shell    # interactive shell in the container
#   make -C linux-cross clean    # remove obj/ and out/

PODMAN ?= podman
IMAGE ?= localhost/bepct4k-build

# Extra flags for the build container, e.g. --userns=keep-id when running
# podman as root. Rootless podman already maps container root to your user,
# so the produced files belong to you.
PODMAN_RUN_FLAGS ?=

# SELinux hosts need :Z to make the bind mount accessible; harmless elsewhere.
MOUNT_OPTS ?= :Z

HERE := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
REPO := $(abspath $(HERE)/..)
STAMP := $(HERE)/.image.stamp

RUN = $(PODMAN) run --rm -v '$(REPO):/work$(MOUNT_OPTS)' -w /work $(PODMAN_RUN_FLAGS)

.PHONY: all fast slow debug image shell clean distclean help

all: fast

fast slow debug: $(STAMP)
	$(RUN) $(IMAGE) $@

# Rebuilt whenever the image definition changes.
$(STAMP): $(HERE)/Containerfile $(HERE)/build.sh
	$(PODMAN) build -t '$(IMAGE)' -f '$<' '$(HERE)'
	@touch '$@'

image:
	$(PODMAN) build -t '$(IMAGE)' -f '$(HERE)/Containerfile' '$(HERE)'
	@touch '$(STAMP)'

shell: $(STAMP)
	$(RUN) -it --entrypoint /bin/bash '$(IMAGE)'

clean:
	rm -rf '$(REPO)/obj' '$(REPO)/out'

distclean: clean
	-$(PODMAN) rmi '$(IMAGE)'
	rm -f '$(STAMP)'

help:
	@awk '/^#/ { sub(/^# ?/, ""); print; next } { exit }' '$(HERE)/Makefile'
