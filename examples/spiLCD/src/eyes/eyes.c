/**
 ****************************************************************************************
 *
 * @file eyes.c
 *
 * @brief Animated Eyes, refer to https://github.com/adafruit/Uncanny_Eyes
 *
 ****************************************************************************************
 */


#if (DISP_EYES)

#include <stdint.h>
#include <stdlib.h>
#include "lcd.h"

#ifndef PROGMEM
#define PROGMEM             __attribute__((aligned(4)))
#endif

// GRAPHICS SETTINGS (appearance of eye) -----------------------------------

// If using a SINGLE EYE, you might want this next line enabled, which
// uses a simpler "football-shaped" eye that's left/right symmetrical.
// Default shape includes the caruncle, creating distinct left/right eyes.

#define SYMMETRICAL_EYELID
#define PIXEL_DOUBLE

// Enable ONE of these #includes -- HUGE graphics tables for various eyes:
//#include "eyes/defaultEye.h"      // Standard human-ish hazel eye -OR-
#include "eyes/dragonEye.h"     // Slit pupil fiery dragon/demon eye -OR-
//#include "eyes/noScleraEye.h"   // Large iris, no sclera -OR-
//#include "eyes/goatEye.h"       // Horizontal pupil goat/Krampus eye -OR-
//#include "eyes/newtEye.h"       // Eye of newt -OR-
//#include "eyes/terminatorEye.h" // Git to da choppah!
//#include "eyes/catEye.h"        // Cartoonish cat (flat "2D" colors)
//#include "eyes/owlEye.h"        // Minerva the owl (DISABLE TRACKING)
//#include "eyes/naugaEye.h"      // Nauga googly eye (DISABLE TRACKING)
//#include "eyes/doeEye.h"        // Cartoon deer eye (DISABLE TRACKING)

// EYE LIST ----------------------------------------------------------------
#define NUM_EYES            (1) // Number of eyes to display (1 or 2)

// INPUT SETTINGS (for controlling eye motion) -----------------------------
#define TRACKING            (1) // If defined, eyelid tracks pupil
#define AUTOBLINK           (1) // If defined, eyes also blink autonomously

#define LIGHT_MIN           0 // Lower reading from sensor
#define LIGHT_MAX           1023 // Upper reading from sensor

#if !defined(IRIS_MIN)      // Each eye might have its own MIN/MAX
#define IRIS_MIN            120 // Iris size (0-1023) in brightest light
#endif
#if !defined(IRIS_MAX)
#define IRIS_MAX            900 // Iris size (0-1023) in darkest light
#endif

#define SPLIT_DUR           8000000L //10000000L //in microseconds

#ifdef PIXEL_DOUBLE
// For the 240x240 TFT, pixels are rendered in 2x2 blocks for an
// effective resolution of 120x120. M0 boards just don't have the
// space or speed to handle an eye at the full resolution of this
// display (and for M4 boards, take a look at the M4_Eyes project
// instead). 120x120 doesn't quite match the resolution of the
// TFT & OLED this project was originally developed for. Rather
// than make an entirely new alternate set of graphics for every
// eye (would be a huge undertaking), this currently just crops
// four pixels all around the perimeter.
#define SCREEN_X_START      4
#define SCREEN_X_END        (SCREEN_WIDTH - 4)
#define SCREEN_Y_START      4
#define SCREEN_Y_END        (SCREEN_HEIGHT - 4)
#else
#define SCREEN_X_START      0
#define SCREEN_X_END        SCREEN_WIDTH
#define SCREEN_Y_START      0
#define SCREEN_Y_END        SCREEN_HEIGHT
#endif

// A simple state machine is used to control eye blinks/winks:
#define NOBLINK             0 // Not currently engaged in a blink
#define ENBLINK             1 // Eyelid is currently closing
#define DEBLINK             2 // Eyelid is currently opening

typedef struct
{
    uint8_t  state;       // NOBLINK/ENBLINK/DEBLINK
    uint32_t duration;    // Duration of blink state (micros)
    uint32_t startTime;   // Time (micros) of last state change
} eyeBlink;

typedef struct _eyes      // One-per-eye structure
{
    eyeBlink     blink;   // Current blink/wink state
//  int wink;
} _eyes;

static _eyes eye[NUM_EYES];

//extern uint32_t _sysTickCnt;
//#define micros() (_sysTickCnt*10000)
extern uint32_t micros(void);

int map(int val, int I_Min, int I_Max, int O_Min, int O_Max)
{
    int a=(val-I_Min);
    int b=(O_Max-O_Min);
    int c=(I_Max-I_Min);

    int ret=((a*100*b*100)/(c*100))/100+O_Min;
    return ret;
}

// Simple Linear Congruential Generator (avoid malloc from newlib rand())
static uint32_t lcg_state = 12345;
static uint32_t random_range(int min, int max) {
    lcg_state = lcg_state * 1103515245 + 12345;
    return (lcg_state % max) + min;
}
#ifdef PIXEL_DOUBLE
uint32_t pixel_buf[120];
#else
uint16_t pixel_buf[128];
#endif

void drawEye( // Renders one eye.  Inputs must be pre-clipped & valid.
    uint8_t  e,       // Eye array index; 0 or 1 for left/right
    uint16_t iScale,  // Scale factor for iris (0-1023)
    uint16_t  scleraX, // First pixel X offset into sclera image
    uint16_t  scleraY, // First pixel Y offset into sclera image
    uint8_t  uT,      // Upper eyelid threshold value
    uint8_t  lT)      // Lower eyelid threshold value
{

    (void)e;
    uint8_t  screenX, screenY;
    uint16_t scleraXsave;
    int16_t  irisX, irisY;
    uint32_t p, a;
    uint32_t d;
    uint16_t max_d = 0;
    uint16_t max_a = 0;
    uint16_t min_d = 0xff;
    uint16_t min_a = 0xff;

    uint32_t  irisThreshold = (128 * (1023 - iScale) + 512) / 1024;
    uint32_t irisScale     = IRIS_MAP_HEIGHT * 65536 / irisThreshold;


    // Set up raw pixel dump to entire screen.  Although such writes can wrap
    // around automatically from end of rect back to beginning, the region is
    // reset on each frame here in case of an SPI glitch.
    // Now just issue raw 16-bit values for every pixel...

    scleraXsave = scleraX; // Save initial X value to reset on each line
    irisY       = scleraY - (SCLERA_HEIGHT - IRIS_HEIGHT) / 2;
    // Lets wait for any previous update screen to complete.

    for (screenY = SCREEN_Y_START; screenY < SCREEN_Y_END; screenY++, scleraY++, irisY++)
    {
        uint32_t idx = 0;
        scleraX = scleraXsave;
        irisX   = scleraXsave - (SCLERA_WIDTH - IRIS_WIDTH) / 2;
        for (screenX = SCREEN_X_START; screenX < SCREEN_X_END; screenX++, scleraX++, irisX++)
        {
            if ((lower[screenY][screenX] <= lT) || (upper[screenY][screenX] <= uT))
            {
                // Covered by eyelid
                p = 0;
            }
            else if ((irisY < 0) || (irisY >= IRIS_HEIGHT) || (irisX < 0) || (irisX >= IRIS_WIDTH))
            {
                // In sclera
                #if (DRAGON_EYE)
                p = 0x0000;
                #else
                p = sclera[scleraY][scleraX];
                #endif
            }
            else
            {
                // Maybe iris...
                p = polar[irisY][irisX];                        // Polar angle/dist
                d = p & 0x7F;                                   // Distance from edge (0-127)
                if (d < irisThreshold)
                {
                    // Within scaled iris area
                    d = d * irisScale / 65536;                    // d scaled to iris image height
                    a = (IRIS_MAP_WIDTH * (p >> 7)) / 512;        // Angle (X)
                    p = iris[d][a];                               // Pixel = iris
                    if (d > max_d) max_d = d;
                    if (a > max_a) max_a = a;
                    if (d < min_d) min_d = d;
                    if (a < min_a) min_a = a;
                }
                else
                {
                    // Not in iris
                    #if (DRAGON_EYE)
                    p = 0x0000;
                    #else
                    p = sclera[scleraY][scleraX];                 // Pixel = sclera
                    #endif
                }
            }

            //p = sclera[screenY][screenX];
            #ifdef PIXEL_DOUBLE
            pixel_buf[idx++] = (p << 16) | p;
            #else
            pixel_buf[idx++] = p;
            #endif
        } // end column

        #ifdef PIXEL_DOUBLE
        //memcpy(pixel_buf[1], pixel_buf[0], 120*sizeof(uint32_t));
        //lcd_fill_pixel(0,0+(screenY-SCREEN_Y_START)*2,240,2, (const uint16_t *)pixel_buf);
        //lcd_wait_done();
        lcd_fill_pixel(0,(screenY-SCREEN_Y_START)*2,240,1, (const uint16_t *)pixel_buf);
        lcd_wait_done();
        lcd_fill_pixel(0,(screenY-SCREEN_Y_START)*2+1,240,1, (const uint16_t *)pixel_buf);
        lcd_wait_done();
        #else
        int offx=(LCD_WIDTH-SCREEN_WIDTH)/2;
        int offy=(LCD_HEIGHT-SCREEN_HEIGHT)/2;

        //DrawImageData(tmp,offx,offy+screenY,SCREEN_WIDTH,1);
        lcd_fill_pixel(offx,offy+screenY,SCREEN_WIDTH,1, pixel_buf);
        lcd_wait_done();
        #endif
    } // end scanline
}

const uint8_t ease[] = { // Ease in/out curve for eye movements 3*t^2-2*t^3
    0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  2,  2,  2,  3,   // T
    3,  3,  4,  4,  4,  5,  5,  6,  6,  7,  7,  8,  9,  9, 10, 10,   // h
   11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 18, 19, 20, 21, 22, 23,   // x
   24, 25, 26, 27, 27, 28, 29, 30, 31, 33, 34, 35, 36, 37, 38, 39,   // 2
   40, 41, 42, 44, 45, 46, 47, 48, 50, 51, 52, 53, 54, 56, 57, 58,   // A
   60, 61, 62, 63, 65, 66, 67, 69, 70, 72, 73, 74, 76, 77, 78, 80,   // l
   81, 83, 84, 85, 87, 88, 90, 91, 93, 94, 96, 97, 98,100,101,103,   // e
  104,106,107,109,110,112,113,115,116,118,119,121,122,124,125,127,   // c
  128,130,131,133,134,136,137,139,140,142,143,145,146,148,149,151,   // J
  152,154,155,157,158,159,161,162,164,165,167,168,170,171,172,174,   // a
  175,177,178,179,181,182,183,185,186,188,189,190,192,193,194,195,   // c
  197,198,199,201,202,203,204,205,207,208,209,210,211,213,214,215,   // o
  216,217,218,219,220,221,222,224,225,226,227,228,228,229,230,231,   // b
  232,233,234,235,236,237,237,238,239,240,240,241,242,243,243,244,   // s
  245,245,246,246,247,248,248,249,249,250,250,251,251,251,252,252,   // o
  252,253,253,253,254,254,254,254,254,255,255,255,255,255,255,255    // n
};

#ifdef AUTOBLINK
uint32_t timeOfLastBlink = 0L, timeToNextBlink = 0L;
#endif

void frame( // Process motion for a single frame of left or right eye
    uint16_t        iScale)       // Iris scale (0-1023) passed in
{
    static uint32_t frames   = 0; // Used in frame rate calculation
    static uint8_t  eyeIndex = 0; // eye[] array counter
    int16_t         eyeX, eyeY;

    if (++eyeIndex >= NUM_EYES) eyeIndex = 0; // Cycle through eyes, 1 per call

    uint32_t        t = micros(); // Time at start of function

    if (!(++frames & 255))   // Every 256 frames...
    {
        //uint32_t elapsed = (millis() - startTime) / 1000;
        //if (elapsed) PRINT(frames / elapsed); // LOG FPS
    }

    // Autonomous X/Y eye motion
    // Periodically initiates motion to a new random point, random speed,
    // holds there for random period until next motion.

    static char  eyeInMotion      = 0;
    static int16_t  eyeOldX = 512, eyeOldY = 512, eyeNewX = 512, eyeNewY = 512;
    static uint32_t eyeMoveStartTime = 0L;
    static int32_t  eyeMoveDuration  = 0L;

    int32_t dt = t - eyeMoveStartTime;      // uS elapsed since last eye event

    if (eyeInMotion)                        // Currently moving?
    {
        if (dt >= eyeMoveDuration)            // Time up?  Destination reached.
        {
            eyeInMotion      = 0;           // Stop moving
            eyeMoveDuration  = random_range(0,3000000); // 0-3 sec stop
            eyeMoveStartTime = t;               // Save initial time of stop
            eyeX = eyeOldX = eyeNewX;           // Save position
            eyeY = eyeOldY = eyeNewY;
        }
        else     // Move time's not yet fully elapsed -- interpolate position
        {
            int16_t e = ease[255 * dt / eyeMoveDuration] + 1;   // Ease curve
            eyeX = eyeOldX + (((eyeNewX - eyeOldX) * e) / 256); // Interp X
            eyeY = eyeOldY + (((eyeNewY - eyeOldY) * e) / 256); // and Y
        }
    }
    else                                    // Eye stopped
    {
        eyeX = eyeOldX;
        eyeY = eyeOldY;

        if (dt > eyeMoveDuration)             // Time up?  Begin new move.
        {
            int16_t  dx, dy;

            do                                  // Pick new dest in circle
            {
                eyeNewX = random_range(0,1024);
                eyeNewY = random_range(0,1024);
                dx      = (eyeNewX * 2) - 1023;
                dy      = (eyeNewY * 2) - 1023;
            }
            while ((uint32_t)(dx * dx + dy * dy) > (1023 * 1023));   // Keep trying
            eyeMoveDuration  = random_range(72000, 144000); // ~1/14 - ~1/7 sec
            eyeMoveStartTime = t;               // Save initial time of move
            eyeInMotion      = 1;            // Start move on next frame
        }
    }


    // Blinking

#ifdef AUTOBLINK
    // Similar to the autonomous eye movement above -- blink start times
    // and durations are random (within ranges).
    if ((t - timeOfLastBlink) >= timeToNextBlink)   // Start new blink?
    {
        timeOfLastBlink = t;
        uint32_t blinkDuration = random_range(36000, 72000); // ~1/28 - ~1/14 sec
        // Set up durations for both eyes (if not already winking)
        for (uint8_t e = 0; e < NUM_EYES; e++)
        {
            if (eye[e].blink.state == NOBLINK)
            {
                eye[e].blink.state     = ENBLINK;
                eye[e].blink.startTime = t;
                eye[e].blink.duration  = blinkDuration;
            }
        }
        timeToNextBlink = blinkDuration * 3 + random_range(8000000,20000000);
    }
#endif

    if (eye[eyeIndex].blink.state)   // Eye currently blinking?
    {
        // Check if current blink state time has elapsed
        if ((t - eye[eyeIndex].blink.startTime) >= eye[eyeIndex].blink.duration)
        {
            // Yes -- increment blink state, unless...
            if(eye[eyeIndex].blink.state==ENBLINK)
            {
                eye[eyeIndex].blink.duration  = random_range(32000, 52000); // DEBLINK is 1/2 ENBLINK speed
                eye[eyeIndex].blink.startTime = t;
                eye[eyeIndex].blink.state=DEBLINK;
            }
            else if (eye[eyeIndex].blink.state == DEBLINK)   // Deblinking finished?
            {
                eye[eyeIndex].blink.state = NOBLINK;      // No longer blinking
            }
        }
    }


    // Process motion, blinking and iris scale into renderable values

    // Scale eye X/Y positions (0-1023) to pixel units used by drawEye()
    eyeX = map(eyeX, 0, 1023, 0, SCLERA_WIDTH  - 128);
    eyeY = map(eyeY, 0, 1023, 0, SCLERA_HEIGHT - 128);

    if (eyeIndex == 1) eyeX = (SCLERA_WIDTH - 128) - eyeX; // Mirrored display

    // Horizontal position is offset so that eyes are very slightly crossed
    // to appear fixated (converged) at a conversational distance.  Number
    // here was extracted from my posterior and not mathematically based.
    // I suppose one could get all clever with a range sensor, but for now...
    if (NUM_EYES > 1) eyeX += 4;
    if (eyeX > (SCLERA_WIDTH - 128)) eyeX = (SCLERA_WIDTH - 128);

    // Eyelids are rendered using a brightness threshold image.  This same
    // map can be used to simplify another problem: making the upper eyelid
    // track the pupil (eyes tend to open only as much as needed -- e.g. look
    // down and the upper eyelid drops).  Just sample a point in the upper
    // lid map slightly above the pupil to determine the rendering threshold.
    static uint16_t uThreshold = 128;
    uint16_t        lThreshold, n;
#ifdef TRACKING
    int16_t sampleX = SCLERA_WIDTH  / 2 - (eyeX / 2), // Reduce X influence
            sampleY = SCLERA_HEIGHT / 2 - (eyeY + IRIS_HEIGHT / 4);
    // Eyelid is slightly asymmetrical, so two readings are taken, averaged
    if (sampleY < 0) n = 0;
    else            n = (upper[sampleY][sampleX] +
                             upper[sampleY][SCREEN_WIDTH - 1 - sampleX]) / 2;
    uThreshold = (uThreshold * 3 + n) / 4; // Filter/soften motion
    // Lower eyelid doesn't track the same way, but seems to be pulled upward
    // by tension from the upper lid.
    lThreshold = 254 - uThreshold;
#else // No tracking -- eyelids full open unless blink modifies them
    uThreshold = lThreshold = 0;
#endif

    // The upper/lower thresholds are then scaled relative to the current
    // blink position so that blinks work together with pupil tracking.
    if (eye[eyeIndex].blink.state)   // Eye currently blinking?
    {
        uint32_t s = (t - eye[eyeIndex].blink.startTime);
        if (s >= eye[eyeIndex].blink.duration) s = 255;  // At or past blink end
        else s = 255 * s / eye[eyeIndex].blink.duration; // Mid-blink
        s          = (eye[eyeIndex].blink.state == DEBLINK) ? 1 + s : 256 - s;
        n          = (uThreshold * s + 254 * (257 - s)) / 256;
        lThreshold = (lThreshold * s + 254 * (257 - s)) / 256;
    }
    else
    {
        n          = uThreshold;
    }

    // Pass all the derived values to the eye-rendering function:
    drawEye(eyeIndex, iScale, eyeX, eyeY, n, lThreshold);
}

uint16_t oldIris = (IRIS_MIN + IRIS_MAX) / 2, newIris;

void split( // Subdivides motion path into two sub-paths w/randimization
    int16_t  startValue, // Iris scale value (IRIS_MIN to IRIS_MAX) at start
    int16_t  endValue,   // Iris scale value at end
    uint32_t startTime,  // micros() at start
    int32_t  duration,   // Start-to-end time, in microseconds
    int16_t  range)      // Allowable scale value variance when subdividing
{

    if (range >= 8)      // Limit subdvision count, because recursion
    {
        range    /= 2;     // Split range & time in half for subdivision,
        duration /= 2;     // then pick random center point within range:
        int16_t  midValue = (startValue + endValue - range) / 2 + random_range(0,range);
        uint32_t midTime  = startTime + duration;
        split(startValue, midValue, startTime, duration, range); // First half
        split(midValue, endValue, midTime, duration, range);     // Second half
    }
    else                 // No more subdivisons, do iris motion...
    {
        int32_t dt;        // Time (micros) since start of motion
        int16_t v;         // Interim value
        while ((dt = (micros() - startTime)) < duration)
        {
            v = startValue + (((endValue - startValue) * dt) / duration);
            if (v < IRIS_MIN)      v = IRIS_MIN; // Clip just in case
            else if (v > IRIS_MAX) v = IRIS_MAX;
            frame(v);        // Draw frame w/interim iris scale value
        }
    }
}

void eyes_loop(void)
{
    newIris = random_range(IRIS_MIN, IRIS_MAX);
    split(oldIris, newIris, micros(), SPLIT_DUR, IRIS_MAX - IRIS_MIN);
    oldIris = newIris;
}

#endif //(DISP_EYES)
