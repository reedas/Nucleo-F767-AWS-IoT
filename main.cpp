/*******************************************************************************
 * Copyright (c) 2014, 2015 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Ian Craggs - make sure QoS2 processing works, and add device headers
 *******************************************************************************/

 /**
 * This is a sample program to illustrate the use of the MQTT Client library
 * on the mbed platform.  The Client class requires two classes which mediate
 * access to system interfaces for networking and timing.  As long as these two
 * classes provide the required public programming interfaces, it does not matter
 * what facilities they use underneath. In this program, they use the mbed
 * system libraries.
 *
 */

 /* This program forms the base platform for the CITY1082 Microprocessors and
  * High Level Languages module Assignment 2 - develop a simple embedded system
  *
  * Andrew Reed - areed@cityplym.ac.uk
  *
  */
#include "mbed-trace/mbed_trace.h"
#include "mbed_events.h"
#include "mbedtls/error.h"
#include "mbed.h"
//#include "blinkThread.h"
//#include "displayThread.h"
//#include "temperatureThread.h"
//#include "ntpThread.h"
#include "AWSThread.h"

//Thread blinkThreadHandle;
//Thread displayThreadHandle;
//Thread temperatureThreadHandle;
//Thread ntpThreadHandle;
Thread AWSThreadHandle;

NetworkInterface *network;



int main()
{
    const float version = 1.0;

    printf("Started System\n");
    ThisThread::sleep_for(100);
    mbed_trace_init();
 
    int ret;
    printf("Opening network interface...\r\n");
    network = NetworkInterface::get_default_instance();
    printf("HelloMQTT: version is %.2f\r\n", version);
    printf("\r\n");

#ifdef MBED_MAJOR_VERSION
    printf("Mbed OS version %d.%d.%d\n\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);
#endif

    printf("Connecting to network\n");
    ret = network->connect();
    if (ret) {
        printf("Unable to connect! returned %d\n", ret);
        return -1;
    }
    printf("Network interface opened successfully.\r\n");
    
    printf("IP address: %s\n", network->get_ip_address());
    printf("Netmask: %s\n", network->get_netmask());
    printf("Gateway: %s\n", network->get_gateway());


//    ntpThreadHandle.start(ntpThread);
//    blinkThreadHandle.start(blinkThread);
//    displayThreadHandle.start(displayThread);
 //   temperatureThreadHandle.start(temperatureThread);
    AWSThreadHandle.start(AWSThread);

    DigitalOut BlueLED(LED2);
    while(1) {
        BlueLED = !BlueLED;
        ThisThread::sleep_for(500);
    }
}
