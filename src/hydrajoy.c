/*
    hydrajoy.c
    
    (c) 2016 GPLv3 Juan Jose Luna Espinosa juanjoluna@gmail.com
    
    This program uses the old Sixense SDK for the Razer Hydra, with its own license (see lib/sixense/README.txt)
    
    See README.md at the root folder for further description.

*/

// Includes

#include <inttypes.h>
#include <time.h>
#include <math.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <signal.h>

#include <linux/input.h>
#include <linux/uinput.h>

#include "../lib/sixense/include/sixense.h"

// Global variables and defines

static volatile int keepRunning = 1;

#define HYDRAJOY_JOYSTICK_NAME "Hydrajoy virtual joystick"

#define NUM_AXIS_CONTROLLER 10
#define NUM_AXIS_TOTAL ( 2 * NUM_AXIS_CONTROLLER )

#define NUM_BUTTONS_CONTROLLER 12
#define NUM_BUTTONS_TOTAL ( 2 * NUM_BUTTONS_CONTROLLER )

#define POSITION_RANGE 4.0f
#define MAX_AXIS_VALUE 32767

#define FIRST_BUTTON_ID 0x120


// Num events per write. The +1 is for the synchronization event
#define NUM_EVENTS_TOTAL ( NUM_AXIS_TOTAL + NUM_BUTTONS_TOTAL + 1 )


// Function prototypes

int initHydra( void );
void finishHydra( void );
void printControllerData( sixenseControllerData * data );

int initUInput( void );
void finishUInput( int fd );

int fillHydraEvents( sixenseAllControllerData *allControllerData, struct input_event *eventBuffer );
void fillAbsEvent( struct input_event *event, int code, int value );
void fillKeyEvent( struct input_event *event, int code, int value );
void fillSyncEvent( struct input_event *event );
void fillPositionalAxisEvent( struct input_event *event, int code, float value );
void fillRawAxisEvent( struct input_event *event, int code, float value );
    
void waitNanoseconds( long nanos );
void signalHandler( int notUsed );


// Main

int main(int argc, char ** argv) {
    
    printf("Start.\n");
    
    // Register signal handler for exiting
    signal(SIGINT, signalHandler);

    // Init Razer Hydra library
    if ( ! initHydra() ) {
        return 0;
    }

    // Create the virtual joystick
    int fd = initUInput();
    if ( fd == -1 ) {
        finishHydra();
        return 0;
    }

    sixenseAllControllerData allControllerData;
    struct input_event eventBuffer[ NUM_EVENTS_TOTAL ];
    struct input_event dummy;
    int numEvents;
    int numBytes;
    while ( keepRunning ) {
    
        // Poll Razer Hydra for data
        sixenseGetAllNewestData( &allControllerData );

        // Fill the events to produce
        numEvents = fillHydraEvents( &allControllerData, eventBuffer );

        // Write the events
        numBytes = numEvents * sizeof( dummy );
        write( fd, eventBuffer, numBytes );

        // Frequency 50 Hz, period 20 ms
        waitNanoseconds( 20000000L );

    }    
    
    finishUInput( fd );

    finishHydra();

    printf("End.\n");

    return 0;
}

// Function bodies

int initHydra( void ) {
    
    if ( sixenseInit() == -1 ) {
        printf("Could not init the Razer Hydra Library.\n");
        return 0;
    }
    else {
        printf("Razer Hydra library inited successfully.\n");
    }
    
    // Wait needed to detect the bases
    waitNanoseconds( 500000000L );
    
    int maxBases = sixenseGetMaxBases();
    printf("Max bases = %d\n", maxBases);
    
    int connectedBase = -1;
    int i;
    for ( i = 0; i < maxBases; i++ ) {

        int connected = sixenseIsBaseConnected( i );

        if ( connected ) {
            connectedBase = i;
            sixenseSetActiveBase( connectedBase );
            printf( "Connected to Razer Hydra base number %d\n", i );
            break;
        }
    }
    if ( connectedBase == -1 ) {
        printf("Could not detect any connected Razer Hydra base. Are you sure it is connected?\n");
        return 0;
    }

    // Success
    return 1;
}

void finishHydra( void ) {
    if ( sixenseExit() == -1 ) {
        printf("There was an error finishing the Razer Hydra.\n");
    }
}

int initUInput( void ) {
    
    char *uinputPath1 = "/dev/input/uinput";
    char *uinputPath2 = "/dev/uinput";
    
    int fd = open( uinputPath1, O_WRONLY | O_NONBLOCK);
    
    if ( fd == -1 ) {
        
        fd = open( uinputPath2, O_WRONLY | O_NONBLOCK);
    
        if ( fd == -1 ) {
            printf( "Could not open uinput file. Tried with /dev/uinput and /dev/input/uinput\n" );
            return -1;
        }
    }
    
    // Inform that we want to produce KEY, ABSolute axis and SYNc events
    int result;
    result = ioctl( fd, UI_SET_EVBIT, EV_KEY );
    result = ioctl( fd, UI_SET_EVBIT, EV_ABS );
    result = ioctl( fd, UI_SET_EVBIT, EV_SYN );

    // Inform for what buttons we want to produce events
    int i;
    for ( i = 0; i < NUM_BUTTONS_TOTAL; i++ ) {
        result = ioctl( fd, UI_SET_KEYBIT, FIRST_BUTTON_ID + i );
    }

    // Inform for what axis we want to produce events
    for ( i = 0; i < NUM_AXIS_TOTAL; i++ ) {
        result = ioctl( fd, UI_SET_ABSBIT, i );
    }

    // Define the virtual joystick device structure
    struct uinput_user_dev userDev;
    memset( &userDev, 0, sizeof(userDev) );
    snprintf( userDev.name, UINPUT_MAX_NAME_SIZE, HYDRAJOY_JOYSTICK_NAME );
    userDev.id.bustype = BUS_USB;
    userDev.id.vendor  = 0x1234;
    userDev.id.product = 0xfedc;
    userDev.id.version = 1;
    
    // For each axis, reister min and max values
    int axisNumber = 0;
    for ( i = 0; i < NUM_AXIS_TOTAL; i++ ) {
        userDev.absmax[ axisNumber ] = MAX_AXIS_VALUE;
        userDev.absmin[ axisNumber++ ] = -MAX_AXIS_VALUE;
    }

    // Write the config structure
    result = write( fd, &userDev, sizeof(userDev));

    // Create the virtual joystick
    result = ioctl( fd, UI_DEV_CREATE );

    return fd;
}

void finishUInput( int fd ) {

    int result = ioctl( fd, UI_DEV_DESTROY );

    if ( close(fd) ) {
        printf("There was an error closing the uinput file\n");
    }

}

int fillHydraEvents( sixenseAllControllerData *allControllerData, struct input_event *eventBuffer ) {
    
    // Returns the number of events filled in

    int numEvents = 0;
    struct input_event *currentEvent = eventBuffer;

    // First generate the buttons events
    int i;
    int buttonIndex = FIRST_BUTTON_ID;
    for ( i = 0; i < 2; i++ ) {
        
        sixenseControllerData *data = &allControllerData->controllers[ i ];

        fillKeyEvent( currentEvent++, buttonIndex++, ( data->buttons & 1 ) != 0 ? 1 : 0 );
        fillKeyEvent( currentEvent++, buttonIndex++, ( data->buttons & 32 ) != 0 ? 1 : 0 );
        fillKeyEvent( currentEvent++, buttonIndex++, ( data->buttons & 64 ) != 0 ? 1 : 0 );
        fillKeyEvent( currentEvent++, buttonIndex++, ( data->buttons & 8 ) != 0 ? 1 : 0 );
        fillKeyEvent( currentEvent++, buttonIndex++, ( data->buttons & 16 ) != 0 ? 1 : 0 );
        fillKeyEvent( currentEvent++, buttonIndex++, ( data->buttons & 256 ) != 0 ? 1 : 0 );
        fillKeyEvent( currentEvent++, buttonIndex++, ( data->buttons & 128 ) != 0 ? 1 : 0 );
        fillKeyEvent( currentEvent++, buttonIndex++, data->is_docked ? 0 : 1 );
        fillKeyEvent( currentEvent++, buttonIndex++, ( data->joystick_x < -0.5 ) ? 1 : 0 );
        fillKeyEvent( currentEvent++, buttonIndex++, ( data->joystick_x > 0.5 ) ? 1 : 0 );
        fillKeyEvent( currentEvent++, buttonIndex++, ( data->joystick_y < -0.5 ) ? 1 : 0 );
        fillKeyEvent( currentEvent++, buttonIndex++, ( data->joystick_y > 0.5 ) ? 1 : 0 );
        numEvents += 12;
    }
    
    // Generate axis events
    int axisIndex = 0;
    for ( i = 0; i < 2; i++ ) {

        sixenseControllerData *data = &allControllerData->controllers[ i ];

        fillPositionalAxisEvent( currentEvent++, axisIndex++, data->pos[ 0 ] );
        fillPositionalAxisEvent( currentEvent++, axisIndex++, data->pos[ 1 ] );
        fillPositionalAxisEvent( currentEvent++, axisIndex++, data->pos[ 2 ] );
        
        fillRawAxisEvent( currentEvent++, axisIndex++, data->rot_quat[ 0 ] );
        fillRawAxisEvent( currentEvent++, axisIndex++, data->rot_quat[ 1 ] );
        fillRawAxisEvent( currentEvent++, axisIndex++, data->rot_quat[ 2 ] );
        fillRawAxisEvent( currentEvent++, axisIndex++, data->rot_quat[ 3 ] );
        
        fillRawAxisEvent( currentEvent++, axisIndex++, data->trigger );
        
        numEvents+= 8;
    }

    for ( i = 0; i < 2; i++ ) {
        
        sixenseControllerData *data = &allControllerData->controllers[ i ];

        fillRawAxisEvent( currentEvent++, axisIndex++, data->joystick_x );
        fillRawAxisEvent( currentEvent++, axisIndex++, data->joystick_y );

        numEvents+= 2;
    }
    
    // Emit the sync event
    fillSyncEvent( currentEvent );
    numEvents++;
    
    return numEvents;
}

void fillAbsEvent( struct input_event *event, int code, int value ) {
    event->type = EV_ABS;
    event->code = code;
    event->value = value;
}

void fillKeyEvent( struct input_event *event, int code, int value ) {
    event->type = EV_KEY;
    event->code = code;
    event->value = value;
}

void fillSyncEvent( struct input_event *event ) {
    event->type = EV_SYN;
    event->code = SYN_REPORT;
    event->value = 0;
}

void fillPositionalAxisEvent( struct input_event *event, int code, float value ) { 
    // Convert mm to meters and divide by position range
    value *= 0.001f / POSITION_RANGE;
    int valueInt = (int)( value * MAX_AXIS_VALUE );
    fillAbsEvent( event, code, valueInt );
}

void fillRawAxisEvent( struct input_event *event, int code, float value ) { 
    int valueInt = (int)( value * MAX_AXIS_VALUE );
    fillAbsEvent( event, code, valueInt );
}

void printControllerData( sixenseControllerData * data ) {
    
    if ( data->enabled && !data->is_docked ) {
    
        printf("Contr %d buttons %d\n", data->controller_index, data->buttons );
        
    }
    
}

void waitNanoseconds( long nanos ) {
    struct timespec timeToWait;
    timeToWait.tv_sec = 0;
    timeToWait.tv_nsec = nanos;
    nanosleep( &timeToWait, NULL );
}

void signalHandler( int notUsed ) {
    keepRunning = 0;
}
