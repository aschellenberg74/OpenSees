/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
**                                                                    **
**                                                                    **
** (C) Copyright 1999, The Regents of the University of California    **
** All Rights Reserved.                                               **
**                                                                    **
** Commercial use of this program without express permission of the   **
** University of California, Berkeley, is strictly prohibited.  See   **
** file 'COPYRIGHT'  in main directory for information on usage and   **
** redistribution,  and for a DISCLAIMER OF ALL WARRANTIES.           **
**                                                                    **
** Developed by:                                                      **
**   Frank McKenna (fmckenna@ce.berkeley.edu)                         **
**   Gregory L. Fenves (fenves@ce.berkeley.edu)                       **
**   Filip C. Filippou (filippou@ce.berkeley.edu)                     **
**                                                                    **
** ****************************************************************** */

#ifndef _SocketStream
#define _SocketStream

// Written: Andreas Schellenberg (andreas.schellenberg@gmail.com)
// Created: 03/23
// Revision: A
//
// Description: This file contains the class definition for SocketStream.

#include <OPS_Stream.h>
#include <Vector.h>

class Channel;


class SocketStream : public OPS_Stream
{
public:
    // constructors
    SocketStream(unsigned int other_Port,
        const char* other_InetAddr = 0,
        bool udp = false,
        bool checkEndianness = false);
    SocketStream();
    
    // destructor
    ~SocketStream();
    
    //int setFile(const char *fileName, openMode mode = OVERWRITE);
    int open();
    int close();
    
    // xml stuff
    int tag(const char*);
    int tag(const char*, const char*);
    int endTag();
    int attr(const char* name, int value);
    int attr(const char* name, double value);
    int attr(const char* name, const char* value);
    int write(Vector& data);
    
    // regular stuff
    OPS_Stream& write(const char* s, int n);
    OPS_Stream& write(const unsigned char* s, int n);
    OPS_Stream& write(const signed char* s, int n);
    OPS_Stream& write(const void* s, int n);
    OPS_Stream& operator<<(char c);
    OPS_Stream& operator<<(unsigned char c);
    OPS_Stream& operator<<(signed char c);
    OPS_Stream& operator<<(const char* s);
    OPS_Stream& operator<<(const unsigned char* s);
    OPS_Stream& operator<<(const signed char* s);
    OPS_Stream& operator<<(const void* p);
    OPS_Stream& operator<<(int n);
    OPS_Stream& operator<<(unsigned int n);
    OPS_Stream& operator<<(long n);
    OPS_Stream& operator<<(unsigned long n);
    OPS_Stream& operator<<(short n);
    OPS_Stream& operator<<(unsigned short n);
    OPS_Stream& operator<<(bool b);
    OPS_Stream& operator<<(double n);
    OPS_Stream& operator<<(float n);
    
    int sendSelf(int commitTag, Channel& theChannel);
    int recvSelf(int commitTag, Channel& theChannel,
        FEM_ObjectBroker& theBroker);

private:
    int sendSize;
    Vector data;
    Channel* theChannel;
};

#endif
