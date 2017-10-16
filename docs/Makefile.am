AUTOMAKE_OPTIONS = subdir-objects
dist_man_MANS = xwmfs.1
EXTRA_DIST = xwmfs.asciidoc

xwmfs.1: xwmfs.asciidoc
	a2x --doctype manpage --format manpage $<
