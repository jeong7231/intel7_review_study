#sudo apt-get install libmariadb-dev-compat
#sudo vi /etc/mysql/mariadb.conf.d/50-server.cnf bind-address 주석
#sudo service mysql restart
gcc iotdb_select.c -o iotdb_select -lmysqlclient
gcc iotdb_insert.c -o iotdb_insert -lmysqlclient
