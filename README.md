## retcon: A cross-platform Twitter client

### Features
* Multiple accounts  
* DM support  
* Post tweets, reply, re-tweet, favourite, follow, etc.  
* Display inline tweet replies, optionally loading more on demand  
* Filtering of incoming and stored tweets  
* Highlighting and hiding of tweets  
* Image preview (can be hidden on a per-tweet basis)  
* Optionally unhiding images previews only temporarily  
* Image display and download  
* User profile display  
* Lookup of user timelines, favourites, followers, etc.  
* Multi-window, tabbed and/or split view  
* Custom combinations of timelines/etc. per panel  
* Assign tweets to custom panels manually or by filter  
* Customisable display formatting  
* Configurable network and storage/caching behaviour  
* Connection failure resilience  
* Emoji display support  
* ...  

### URLs
This project is currently hosted at:  
https://github.com/JGRennison/retcon

This project was previously hosted at:  
https://bitbucket.org/JGRennison/retcon

Windows binaries are available at:  
https://github.com/JGRennison/retcon/releases

### Documentation
See doc/ directory.  
This includes documentation for:  

* Filter syntax and examples  
* Tweet flags  
* Tweet display format codes  
* Command line switches  

### Build dependencies
* SQLite v3.6 or later  
* PCRE  
* wxWidgets v2.8 series  
* zlib  
* libcurl v7.21.6 or later with SSL  
* Boost.Iterator  
* libvlc (except on Windows)  

### Building
Should build out of the box with `make` and `make install`.  
Requires a relatively recent gcc (4.7.1 or later).  
Building for Windows involves a lot of hoop-jumping. See doc/building_on_windows.txt for details.  
Windows users are encouraged to download pre-built binaries from GitHub (see above).  

### Credits
The following libraries are bundled with the source:

* [utf8proc](http://www.public-software-group.org/utf8proc)  
* [rapidjson](https://code.google.com/p/rapidjson/)  
* [twitcurl](http://code.google.com/p/twitcurl/) (modified)  
* [SimpleOpt](http://code.google.com/p/simpleopt/) (modified)  
* [cpp-btree](https://code.google.com/p/cpp-btree/) (modified)  
* [twemoji](https://github.com/twitter/twemoji/) (partial)  

### License:
GPLv2
