RT viewer is a technology preview for RawTherapee.

The extensive use of OO with C++ can facilitate programmers
to keep their focus on functionality where most of the 'hard work' like
color transformations and memory management is done on the objects.

version 0.0.0

building:

  make rtviewer

works on:

  Ubuntu/Debian with 32bpp X desktop

depends on:

  $HOME/.config/RawTherapeeAlpha/profiles/default.pp3
  it will default to this pp3 file for raw files without pp3 file.

requirements:

	liblcms2
	Xlibraries

	builds with -lXext -llcms2 -lgomp -lX11

(c) 2011 Jan Rinze Peterzon janrinze@gmail.com
 