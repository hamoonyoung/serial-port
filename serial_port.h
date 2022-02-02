/* 
* @license Moon Young Ha <https://github.com/hamoonyoung/serial_port>
*
* References:
* https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
* https://www.cmrr.umn.edu/~strupp/serial.html
* https://pbxbook.com/other/mac-tty.html
*/

#ifndef SERIAL_PORT_H_GUARD
#define SERIAL_PORT_H_GUARD

void * send (void * serialPort);

void * receive (void * serialPort);

int open_port(char * serialPort);


#endif