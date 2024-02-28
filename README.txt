CS342-Project 1
Ahmet Alperen Yılmazyıldız 22002712
Zeynep Doğa Dellal 22002572


This project is executing interactive mode, batch mode, two command inputs using | correctly.
It produces a temporary result file, the name of the file is the clients connection number.
Multiple clients can connect and get correct results.

The structure and execution is done according to the instructions.

**HOW TO RUN**
In first terminal: make
In first terminal: ./comserver /MQ_NAME
In second terminal: ./comcli /MQ_NAME -s WSIZE

For batch mode: ./comcli /MQ_NAME -b file_name

To see result look at .txt file that is generated with the client number