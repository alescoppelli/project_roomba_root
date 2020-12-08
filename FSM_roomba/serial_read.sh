#!/bin/sh

SERIAL_PORT=/dev/ttyUSB0

view_serial_port(){

    echo "Serial port: $SERIAL_PORT"
}


view_serial_configuration(){
    printf "Serial configuration of $SERIAL_PORT\n"
    printf "\n"
    stty -F $SERIAL_PORT -a
    printf "\n"
    
}



# What the arguments mean:
#   cs8:     8 data bits
#   -parenb: No parity (because of the '-')
#   -cstopb: 1 stop bit (because of the '-')
#   -echo: Without this option, Linux will sometimes automatically send back
#          any received characters, even if you are just reading from the serial
#          port with a command like 'cat'. Some terminals will print codes
#          like "^B" when receiving back a character like ASCII ETX (hex 03).

serial_configuration(){

  stty -F /dev/ttyUSB0 115200 cs8 -parenb -cstopb 
}    

#view_serial_port
view_serial_configuration
serial_configuration
view_serial_configuration
