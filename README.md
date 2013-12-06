ogeo -  Open GeoIP
===

Console app to download geoip database to automate creation of sqlite3 db

end result: 149M geoipDB.db3

```
macpro:~ dev$ ogeo
Usage: ogeo (Options)
 {1} Options:
	--download or -d : take all csv from geoip location and info
	--handle or -r : read and convert table to sqlite3 db. +150mb data
	-ip 174.36.207.186 : query db sqlite3
	--interactive or -c : start interactive modus on sqlite3 db. and play chat your query
	--version or -V: show the version of conversion used
	--help or -h: display this text.
macpro:~ dev$ ogeo -c
**********************************************************************************************************************************
Enter SQL statements multiline terminated with a ";"
Enter "-help" for instructions -q to quit
Comand or query:
sql>-h
Last cmd:-h
-table  or -t          Show current Table
-dump ?TABLE? ...      Dump the database in an SQL text format
                         If TABLE specified, only dump tables matching
                         LIKE pattern TABLE.
-sp                    Update country if having table geoipcountrywhois + geolocation
-vacum                 VACUUM to db long time by 100Mb file
-lib                   Display sqlite3 version on this app.
-help or -f1 or -h     Show this message
-exit or -quit or -q   Exit this modus

**********************************************************************************************************************************
Enter SQL statements multiline terminated with a ";"
Enter "-help" for instructions -q to quit
Comand or query:
sql>


to build on qt5  http://qt-project.org/
git clone https://github.com/pehohlva/ogeo.git
cd ogeo
qmake && make  
make install on mac or linux

```
