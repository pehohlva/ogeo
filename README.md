ogeo -  Open GeoIP
===

Console app to download geoip database to automate creation of sqlite3 db

end result: 149M geoipDB.db3

...

./ogeo
Usage: ./ogeo (Options)
 {1} Options:
	--download or -d : take all csv from geoip location and info
	--handle or -r : read and convert table to sqlite3 db.
	--handle or -ip 174.36.207.186 : query db sqlite3
	--version or -V: show the version of conversion used
	--help or -h: display this text.
... 

to build on qt5  http://qt-project.org/
git clone https://github.com/pehohlva/ogeo.git
cd ogeo
qmake && make  
