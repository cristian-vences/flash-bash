/*
 * flash_bash.c
 *
 * Flash BASH is a Raspberry Pi based tool to automate glitching into privileged shells like bootloader shells.
 * This code utilizes the pigpio library to interface with the RPi's GPIO pins that will be used either as an input trigger or output trigger to initiate glitching.
 * Please visit https://github.com/cristian-vences/flash-bash for more detail.
 * 
 * Author: Cristian Vences
 * Updated September 2024
 */

#include <stdio.h> //getchar,printf
#include <stdlib.h>
#include <unistd.h>
#include <string.h> //strlen
#include <errno.h>	//strerror

#include <pigpio.h>	//pigpio library

#define PIN7 4       //pin for glitching, broadcom number 4
#define PIN11 17     //pin for glitching, broadcom number 17

#define TIMED 1      //number definition
#define SERIAL 2     //number definition

#define SHOW_OUTPUT  //undefine to not show serial data on stdout


int main(void)
{
	// Initialization variables
	unsigned version;			//store pigpio version
	unsigned int hwRevision;	//store hardware revision
	char hwBuff[32];			//buffer for formatting unsigned int in hex
	int gpioResult = 0;			//store result from pigpio calls

	// Configuration variables
	int choice;				 	//for choosing glitch style
	int attackType = 0;	 		//attack style

	// Serial attack variables
	int fd;						//file descriptor
	char startTrigger[100];		//glitch start trigger string
	char stopTrigger[100];		//glitch stop trigger string
	char buff[100];				//buffer for incoming serial characters
	char device[100];			//device name from /dev
	int i, n, n2, in;		 	//for loops, for stringlength, for storing incoming serial
	int baud;					//stores baud rate
	int val = 1;			 	//value from string comparison
	int count = 0;			 	//simple count for how many times buff has been changed

	// Time attack variables
	int startTime = 0;		 	//time for startup before glitching
	int stopTime = 0;		 	//time for tool to stop glitching

	// Welcome
	printf("Welcome to Flash BASH!\n\n");
	printf("                       #############:: ###########                         \n");
	printf("                       ########### ::: *##########                         \n");
	printf("                       ########## :::: #####*#####                         \n");
	printf("                       ######     ::::::::::*##*##                         \n");
	printf("                       ####*#   :::-::--::: *#####                         \n");
	printf("                       ######  ::::::-::::  ######                         \n");
	printf("                       ###### ::::--:::::   ######                         \n");
	printf("                       ######-::::::::::    ######                         \n");
	printf("                       ########### :::: ##########                         \n");
	printf("                       ########### ::: ###########                         \n");
	printf("                       ########### :: ############                         \n");
	printf("                                                                           \n");
	printf("             :::::     :            ::         ::       ::   :             \n");
	printf("            ::::::    -::          :::-      ::::::     ::  :::            \n");
	printf("            :::::     -::         :::::      ::::::     :::::::            \n");
	printf("            :::       -::         ::::::     ::-:::     ::  :::            \n");
	printf("            :::        ::::::    ::   :::    :::::      ::  :::            \n");
	printf("                                                                           \n");
	printf(" ###############         ######          ###############*  #####      #####\n");
	printf("#################      #########       ################    #####      #####\n");
	printf("            #####     #*#########      #####               ################\n");
	printf("################     ###### *######     #################  ################\n");
	printf("#####       ####*   #####     #####*                 ####* #####******#####\n");
	printf("################# ###### ############    ################# #####      #####\n");
	printf("################ #####*################*################*  #####      #####\n");

	// Print versions
	printf("\n****CHECKING VERSIONS*****\n\n");
	version = gpioVersion(); 	//call for pigpio version
	printf("Using pigpio version: %u\n", version);

	hwRevision = gpioHardwareRevision();	//call for hardware revision https://abyz.me.uk/rpi/pigpio/cif.html#gpioHardwareRevision
	snprintf(hwBuff, sizeof(hwBuff), "%x", hwRevision);	//securely format the hardware revision as a hex string
	printf("Running on: %s\n", hwBuff);

	// Initialize pigpio
	printf("Initializing pigpio. . .");
	gpioResult = gpioInitialise();

	if (gpioResult == PI_INIT_FAILED) {
        printf("ERROR\n Unable to initialize pigpio.\n");
		printf("Error value = %d\n", gpioResult);
        return -1;  // Return -1 value to indicate an error
    }

	printf(" SUCCESS\n");

	// Begin configuration
	printf("\n****CONFIGURATION*****\n\n");

	// Get attack style selection from user
	printf("What type of attack? TIMED [1] or SERIAL [2]: ");
	scanf("%d", &choice);
	if ((choice != 1) && (choice != 2))
	{
		printf("Invalid selection, expect 1 or 2.\n");
		return -1;
	}
	else if (choice == 1)
	{
		attackType = TIMED;
		printf("Attack style: TIMED\n\n");
	}
	else
	{
		attackType = SERIAL;
		printf("Attack style: SERIAL\n\n");
	}

	// Configure the GPIO pin attached to the MUX as an output pin
	// PI_OUTPUT is defined within the pigpio library
	gpioResult = gpioSetMode(PIN7, PI_OUTPUT);

	// Check the result to ensure the GPIO pin could be properly configured
	if (gpioResult != 0) // A value of 0 = OK
	{
		// Error handling
		switch (gpioResult)
		{
			case PI_BAD_GPIO:
				printf("%d is a bad gpio pin\n", PIN7);
				return -1;

			case PI_BAD_MODE:
				printf("Bad mode specified for gpio pin %d\n", PIN7);
				return -1;

			default:
				printf("Unexpected error encountered when setting mode specified for gpio pin %d\n", PIN7);
				printf("Result = %d\n", gpioResult);
				return -1;
		}
	}

	// Initially set PIN7 to high
	gpioWrite(PIN7, PI_ON);

	// Go through set up for serial attack
	if (attackType == SERIAL)
	{
		// Get Baud rate from user
		printf("What BAUD (9600, 115200, 38400, etc): ");
		scanf("%d", &baud);
		if ((baud <= 0) && (baud >= 250001))
		{
			printf("Invalid baud rate entry (expects 1 to 250000 baud).\n");
			return -1;
		}
		else
		{
			printf("Baud = %d\n\n", baud);
		}

		// Get serial device from user
		printf("Enter serial device descriptor: ");
		scanf("%99s", device);
		printf("Device descriptor = `%s`\n\n", device);

		// Initialize serial communication
		if ((fd = serOpen(device, baud, 0)) < 0) // A value of 0 >= OK
		{
			// Error handling
			switch (fd)
			{
				case PI_NO_HANDLE:
					printf("No handle available: %d\n", fd);
					return -1;

				case PI_SER_OPEN_FAILED:
					printf("Can't open serial device: %d\n", fd);
					return -1;

				default:
					printf("Unexpected error encountered when setting mode specified for gpio pin %d\n", fd);
					printf("Result = %d\n", gpioResult);
					return -1;
			}
		}

		// Obtain start trigger for glitching from user
		printf("What phrase would you like to start glitching on? (no longer than 99 characters):\n");
		scanf("%99s", startTrigger);
		n = strlen(startTrigger);
		printf("Trigger string length = %d\n", n);
		printf("Glitch string = '%s'\n\n", startTrigger);

		// Obtain stop trigger for glitching from user
		printf("What phrase would you like to stop glitching on? (no longer than 99 characters):\n");
		scanf("%99s", stopTrigger);
		n2 = strlen(stopTrigger);
		printf("Trigger tirng length = %d\n", n2);
		printf("Glitch string = '%s'\n", stopTrigger);

		// This buffer is filled by the serial output from target device.
		// The buffer is compared to startTrigger and stopTrigger. All 3 are of size 100.
		// Once buffer is equal to a trigger it either grounds the pin or releases

		// Fill in buffer once from serial
		for (i = 0; i < n; i++)
		{
			// Begin reading from serial input
			if ((in = serReadByte(fd)) < 0) // A value of 0 >= is a legit character
			{
				// Error handling
				switch (in)
				{
					case PI_BAD_HANDLE:
						printf("Bad handle provided: %d\n", in);
						return -1;

					case PI_SER_READ_FAILED:
						printf("Serial read failed: %d\n", in);
						return -1;

					case PI_SER_READ_NO_DATA:
						break;

					default:
						printf("Unexpected error encountered when reading byte: %d\n", in);
						return -1;
				}
			} else {
				buff[i] = in;
			}
		}
		buff[i] = '\0';
#ifdef SHOW_OUTPUT
		printf("%d: buff = %s\n", count, buff);
#endif
		val = strcmp(buff, startTrigger); //compare the buffer to the trigger

		// Start loop to compare strings
		// NOTE: This code could be made faster for efficiency and accuracy of
		//       triggering, however in basic glitching scenarios this is good enough.
		while (val != 0)
		{ //run loop until buff and trigger are the same
			if ((in = serReadByte(fd)) < 0) // A value of 0 >= is a legit character
			{
				// Error handling
				switch (in)
				{
					case PI_BAD_HANDLE:
						printf("Bad handle provided: %d\n", in);
						return -1;

					case PI_SER_READ_FAILED:
						printf("Serial read failed: %d\n", in);
						return -1;

					case PI_SER_READ_NO_DATA:
						break;

					default:
						printf("Unexpected error encountered when reading byte: %d\n", in);
						return -1;
				}
			} else {
				fflush(stdout);
				// This loop 'shifts' the buffer down to make space for the incoming
				// byte, as we do a rolling comparison byte-by-byte.
				for (i = 0; i < n - 1; i++)
				{
					buff[i] = buff[i + 1];
				}
				buff[i] = in;
				buff[i + 1] = '\0';
				count++;
#ifdef SHOW_OUTPUT
			printf("%d: buff = %s\n", count, buff);
#endif
			// If startTrigger == buff, break out of this loop:
			val = strcmp(buff, startTrigger);
			}
		}

		// Print success statement and trigger glitching
		printf("\n\n\n\n\nGLITCHING INITIATED!\n\n\n\n\n"); //buff and trigger ==
		gpioWrite(PIN7, PI_OFF);  //trigger MUX
		val = 1;    //reset loop breaker
		count = 0;  //reset buffer count

		// Begin comparing target output to stopTrigger to cease glitching
		// Fill in buffer once from serial
		for (i = 0; i < n2; i++)
		{
			if ((in = serReadByte(fd)) < 0) // A value of 0 >= is a legit character
			{
				// Error handling
				switch (in)
				{
					case PI_BAD_HANDLE:
						printf("Bad handle provided: %d\n", in);
						return -1;

					case PI_SER_READ_FAILED:
						printf("Serial read failed: %d\n", in);
						return -1;

					case PI_SER_READ_NO_DATA:
						break;

					default:
						printf("Unexpected error encountered when reading byte: %d\n", in);
						return -1;
				}
			} else {
				buff[i] = in;
			}
		}
		buff[i] = '\0';
#ifdef SHOW_OUTPUT
		printf("%d: buff = %s\n", count, buff);
#endif
		val = strcmp(buff, stopTrigger);  //compare the buffer to the trigger

		// Start loop to compare strings
		while (val != 0)
		{ //run loop until buff and trigger are the same
			if ((in = serReadByte(fd)) < 0) // A value of 0 >= is a legit character
			{
				switch (in)
				{
					case PI_BAD_HANDLE:
						printf("Bad handle provided: %d\n", in);
						return -1;

					case PI_SER_READ_FAILED:
						printf("Serial read failed: %d\n", in);
						return -1;

					case PI_SER_READ_NO_DATA:
						break;

					default:
						printf("Unexpected error encountered when reading byte: %d\n", in);
						return -1;
				}
			} else {
				fflush(stdout);
				// This loop 'shifts' the buffer down to make space for the incoming
				// byte, as we do a rolling comparison byte-by-byte.
				for (i = 0; i < n2 - 1; i++)
				{
					buff[i] = buff[i + 1];
				}
				buff[i] = in;
				buff[i + 1] = '\0';
				count++;
#ifdef SHOW_OUTPUT
			printf("%d: buff = %s\n", count, buff);
#endif
			// If stopTrigger == buff, break out of this loop:
			val = strcmp(buff, stopTrigger);
			}
		}

		// Print success statement and trigger glitching
		printf("\n\n\n\nGLITCHED\n\n\n\n"); 	//buff and trigger ==
		gpioWrite(PIN7, PI_ON);				//trigger MUX
		val = 1;
		count = 0;

		// Close serial port
		serClose(fd);
	}
	else
	{
		// Set up timed glitch attack

		// Configure the GPIO pin attached to the target VCC as an input pin
    	// PI_INPUT is defined within the pigpio library
		gpioResult = gpioSetMode(PIN11, PI_INPUT);

		// Check the result to ensure the GPIO pin could be properly configured
	    if (gpioResult != 0) // A value of 0 = OK
   		{
			// Error handling
			switch (gpioResult)
			{
				case PI_BAD_GPIO:
					printf("%d is a bad gpio pin\n", PIN11);
					return -1;

				case PI_BAD_MODE:
					printf("Bad mode specified for gpio pin %d\n", PIN11);
					return -1;

				default:
					printf("Unexpected error encountered when setting mode specified for gpio pin %d\n", PIN11);
					printf("Result = %d\n", gpioResult);
					return -1;
			}
		}

		// Get time variable from user
		printf("How long after boot to start glitching (1 - 300 secs): ");
		scanf("%d", &startTime);
		if ((startTime <= 0) && (startTime >= 301))
		{
			printf("Invalid time entry (expects 1 to 300 secs).\n");
			return -1;
		}
		else
		{
			printf("startTime = %d\n\n", startTime);
		}
		
		printf("How long after glitching has started to stop (1 - 300 secs): ");
		scanf("%d", &stopTime);
		if ((stopTime <= 0) && (stopTime >= 301))
		{
			printf("Invalid time entry (expects 1 to 300 secs).\n");
			return -1;
		}
		else
		{
			printf("stopTime = %d\n\n", stopTime);
		}

		printf("Please turn on target device now!\n\n");

		while (gpioRead(PIN11) != 1)
		{
			// Wait for device to turn on, as PIN11 is sensing device VCC.
		}

		// NOTE: Precision of this is not optimal as OS may not give us accurate timing,
		//		 users who need more precision should update this.		
		sleep(startTime);
		
		gpioWrite(PIN7, PI_OFF); //trigger MUX
		printf("\n\n\n\n\nGLITCHING INITIATED!\n\n\n\n\n"); //buff and trigger ==

		// NOTE: See above (lack of) precision note.
		sleep(stopTime);

		gpioWrite(PIN7, PI_ON);  //trigger MUX
		printf("\n\n\n\nGLITCHING CEASED\n\n\n\n");  //time elapsed
	}

	// Terminate the pigpio library to clean up
    gpioTerminate();

	return 0;
}