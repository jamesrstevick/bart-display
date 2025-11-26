#ifndef CONFIG_H
#define CONFIG_H

/*******************************************************************************
 * BART Display Configuration Header
 * Contains all hardcoded strings, constants, and configuration values
 ******************************************************************************/

// Display Text Sizes
#define SMALL_TXT 7
#define LARGE_TXT 8
#define INSTRUCTION_TXT 3
#define STATION_NAME_TXT 5  // Text size for station names (fits longest name: 28 chars after SFO Airport change)
#define ARRIVAL_STATION_TXT 8  // Text size for arrival station names
#define ARRIVAL_TIME_TXT 7     // Text size for arrival times
#define ARRIVAL_CARS_TXT 6     // Text size for car count

// Array Sizes
#define STAT_ARR_SIZE 55UL
#define JSON_DOC_SIZE 20480UL

// Button Configuration
#define UP_BUTTON 5 
#define DN_BUTTON 6 
#define BUTTON_DEBOUNCE_DELAY 20
#define MED_PRESS_TIME 1250
#define LONG_PRESS_TIME 10000

// Display Strings (all caps)
#define STR_SELECT_STATION "SELECT STATION"
#define STR_PRESS_UP_OR_DOWN "PRESS UP OR DOWN TO SELECT"
#define STR_STATION_SELECTED "STATION SELECTED:"
#define STR_STATION_SELECTED_CONFIRM "STATION SELECTED!"
#define STR_MIN " MIN"
#define STR_CAR_TRAIN " CAR TRAIN"
#define STR_ARRIVING " ARRIVING"
#define STR_SPACE " "
#define STR_COMMA ","

// Serial Messages (all caps)
#define SERIAL_MSG_STATION_SELECTED "STATION SELECTED"
#define SERIAL_MSG_STATIONS "STATIONS: "
#define SERIAL_MSG_CREATED_ARRAY "CREATED ARRAY"
#define SERIAL_MSG_DESTINATION "DESTINATION: "
#define SERIAL_MSG_HTTP_GET_FAILED "HTTP GET FAILED, CODE: "
#define SERIAL_MSG_DESERIALIZE_FAILED "DESERIALIZEJSON() FAILED: "

// BART API Configuration
#define BART_API_KEY "QWMH-PE9Y-9WST-DWE9"
#define BART_API_STATIONS_URL "https://api.bart.gov/api/stn.aspx?cmd=stns&key=QWMH-PE9Y-9WST-DWE9&json=y"
#define BART_API_ETD_URL_BASE "https://api.bart.gov/api/etd.aspx?cmd=etd&orig="

// Display Layout Constants
#define DESTINATION_MAX_LENGTH 11
#define DISPLAY_ROTATION 3  // 3 = horizontal (960x320)
#define DISPLAY_MARGIN 20
#define ARRIVAL_MARGIN 20   // Margin for arrival display (left side)
#define ARRIVAL_DISPLAY_DELAY 10000  // Time to display each pair of arrivals (milliseconds)
#define ARRIVAL_BLANK_DELAY 500     // Blank screen delay between arrival sets (milliseconds)
#define ARRIVING_DISPLAY_DELAY 20000  // Time to display arriving train screen (milliseconds)
#define ARRIVING_STATION_TXT 8  // Text size for arriving train destination
#define ARRIVING_CAR_TXT 5      // Text size for arriving train car count
#define DISPLAY_WIDTH 960   // Horizontal width
#define DISPLAY_HEIGHT 320  // Horizontal height
#define CHAR_WIDTH 6  // Base character width in pixels
#define CHAR_HEIGHT 8 // Base character height in pixels

#endif // CONFIG_H

