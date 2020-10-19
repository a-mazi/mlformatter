mlformatter
===========

*mlformatter* is a markup language file formatter. Its primary goal is
to make the file better suited for version comparison. It is focused on
the HTML as the widest supported document format.

Main features
-------------

-   sentence per line separation

-   HTML header automatic numbering

-   embedding pictures into HTML files

Compilation
-----------

The source code of the program is put into a single file. This is to
make it more portable and easy to deploy on any platform that supports
C++ compilation.

If your platform supports make, to build the program type in a console:

    $ make [release|debug] 

Otherwise, to compile type:

    $ g++ mlformatter.cpp -o mlformatter[.exe] 

Usage
-----

You can use *mlformatter* standalone or as an additional tool in
connection with other program. To use it standalone type in the console:

    $ ./mlformatter <filename.ext> 

*mlformatter* recognizes the file format by its extension. For
unrecognized formats the program does nothing.

Installation
------------

### LibreOffice Writer

You can use the program with the *LibreOffice Writer*. It is a quite
good HTML WYSIWYG editor. It supports also tags like: `<pre>`, `<code>`,
`<kdb>`, `<samp>`.

To setup the *LibreOffice Writer* to run the program after each file
do:\
Menu: Tools -\> Customize -\> Events\
Save in: LibreOffice\
Event: Document has been saved\
Assigned Action: Standard.Module1.Main

    Sub Main
    Dim url As String
    url = ConvertFromURL( ThisComponent.Url )
    Shell "/home/user/mlformatter " & url
    End Sub

### Git

To use it in *git* hooks modify the following file:

.git/hooks/pre-commit

with this content:

    #!/bin/sh
    git diff --cached --name-only --diff-filter=ACM | xargs -n 1 -I '{}' ../tools/mlformatter '{}'
    git diff --cached --name-only --diff-filter=ACM | xargs -n 1 -I '{}' git add '{}'

Alternatively a *gitconfig* can be used like this:

    [filter "formatter"]
        smudge = tools/mlformatter
        clean = tools/mlformatter
