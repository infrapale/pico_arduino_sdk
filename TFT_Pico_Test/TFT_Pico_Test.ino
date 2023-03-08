/*
  Example animated analogue meters using a ILI9341 TFT LCD screen

  Needs Font 2 (also Font 4 if using large scale label)

  Make sure all the display driver and pin connections are correct by
  editing the User_Setup.h file in the TFT_eSPI library folder.

  #########################################################################
  ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
  #########################################################################
*/

#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

#define TFT_GREY 0x5AEB

#define LOOP_PERIOD 35 // Display updates every 35 ms
#define NBR_SECTIONS 4

//#define DEG_TO_RAD  0.0174532925


typedef struct {
  int16_t x;
  int16_t y;
} xy_st;

typedef struct {
  xy_st base;
  xy_st dim;
  xy_st mid;
  xy_st reading_base;
  xy_st reading_dim;
  xy_st reading_text_mid;
  uint16_t fill;
  uint16_t border;
} box_st;

typedef struct {
    int16_t range[3];
    uint16_t color;
} section_st;

typedef struct {
    xy_st       base;
    uint16_t    color;
    uint16_t    len;
    int16_t    range[2];
    int16_t    full_range;
    xy_st       prev_xy[2];
    section_st  section[NBR_SECTIONS];
} needle_st;

typedef struct{
  float max;
  float min;
  float value;
} scale_st;

typedef struct {
    box_st      box;
    char        label[12];
    scale_st    scale;
    needle_st   needle;
} meter_st;


float ltx = 0;    // Saved x coord of bottom of needle
uint16_t osx = 120, osy = 120; // Saved x & y coords
uint32_t updateTime = 0;       // time for next update

int old_analog =  -999; // Value last displayed
int old_digital = -999; // Value last displayed

int value[6] = {0, 0, 0, 0, 0, 0};
int old_value[6] = { -1, -1, -1, -1, -1, -1};
int d = 0;


meter_st meter; 


void setup(void) 
{
  delay(3000);
  tft.init();
  tft.setRotation(3);
  Serial.begin(9600); // For debug
  Serial.println("FTFT_Pico" );
  tft.fillScreen(TFT_BLACK);
  pinMode(TFT_BL, OUTPUT);  
  
  digitalWrite(TFT_BL, HIGH);
  
    meter.box.base.x  = 0;
    meter.box.base.y  = 0;
    meter.box.dim.x   = TFT_HEIGHT;
    meter.box.dim.y   = TFT_WIDTH-80;
    meter.box.mid.x   = meter.box.dim.x / 2;
    meter.box.mid.y   = meter.box.dim.y / 2;
    meter.box.fill    = TFT_WHITE;
    meter.box.border  = ILI9341_MAROON;
    strcpy(meter.label, "Outdoor C");
    meter.scale.min   = -30.0;
    meter.scale.max   = 30.0;
    meter.scale.value = 2.0;

    meter.box.reading_base.x = meter.box.base.x + meter.box.mid.x-50;
    meter.box.reading_base.y = meter.box.base.y + meter.box.mid.y+35;
    meter.box.reading_dim.x = 100;
    meter.box.reading_dim.y = 30;
    meter.box.reading_text_mid.x = meter.box.reading_base.x + meter.box.reading_dim.x /2;
    meter.box.reading_text_mid.y = meter.box.reading_base.y + meter.box.reading_dim.y /8;


    meter.needle.range[0] = 20;
    meter.needle.range[1] = 160;
    meter.needle.range[2] = 5;
    meter.needle.range[3] = meter.needle.range[2] * 4;

    meter.needle.full_range = meter.needle.range[1] -meter.needle.range[0];
    
    meter.needle.section[0].range[0] = meter.needle.range[0] + (meter.needle.full_range * 0);
    meter.needle.section[0].range[1] = meter.needle.range[0] + (meter.needle.full_range * 0.2);
    meter.needle.section[1].range[0] = meter.needle.range[0] + (meter.needle.full_range * 0.2);
    meter.needle.section[1].range[1] = meter.needle.range[0] + (meter.needle.full_range * 0.4);
    meter.needle.section[2].range[0] = meter.needle.range[0] + (meter.needle.full_range * 0.4);
    meter.needle.section[2].range[1] = meter.needle.range[0] + (meter.needle.full_range * 0.7);
    meter.needle.section[3].range[0] = meter.needle.range[0] + (meter.needle.full_range * 0.7);
    meter.needle.section[3].range[1] = meter.needle.range[0] + (meter.needle.full_range * 1.0);

    meter.needle.section[0].color = ILI9341_LIGHTGREY;
    meter.needle.section[1].color = TFT_GREEN;
    meter.needle.section[2].color = TFT_YELLOW;
    meter.needle.section[3].color = TFT_RED;

    meter.needle.base.x = meter.box.mid.x; 
    meter.needle.base.y = meter.box.dim.y * 17 / 16;  
    meter.needle.len    = meter.needle.base.y * 7/10;  
    meter.needle.color  = ILI9341_RED;       

    



  //analogMeter(); // Draw analogue meter

  // Draw 6 linear meters
  /*
  byte d = 40;
  plotLinear((char*)"A0", 0, 160);
  plotLinear((char*)"A1", 1 * d, 160);
  plotLinear((char*)"A2", 2 * d, 160);
  plotLinear((char*)"A3", 3 * d, 160);
  plotLinear((char*)"A4", 4 * d, 160);
  plotLinear((char*)"A5", 5 * d, 160);
  */
  updateTime = millis(); // Next update time
}

void draw_label(meter_st *meter)
{
    tft.setTextColor(TFT_BLACK);  // Text colour
    tft.drawCentreString(meter->label, meter->box.mid.x , meter->box.mid.y, 4); // Comment out to avoid font 4
}

void draw_box(box_st *box)
{
  tft.fillRect(box->base.x, box->base.y, box->dim.x, box->dim.y, box->fill);
  tft.drawRect(box->base.x, box->base.y, box->dim.x, box->dim.y, box->border);
}



void draw_needle(meter_st *meter, int16_t angle )
{
    xy_st  xy0;
    xy0.y = meter->needle.base.y - meter->box.dim.y + 2; 
    if (angle > 90) xy0.x = -xy0.y * tan((90-angle) * DEG_TO_RAD) ;
    else xy0.x = -xy0.y / tan(angle * DEG_TO_RAD);

  tft.drawLine(
        (float) meter->needle.prev_xy[0].x, 
        (float) meter->needle.prev_xy[0].y, 
        (float) meter->needle.prev_xy[1].x, 
        (float) meter->needle.prev_xy[1].y, 
        TFT_WHITE);
    draw_label(meter);  
    meter->needle.prev_xy[0].x=meter->needle.base.x + xy0.x;
    meter->needle.prev_xy[0].y=meter->needle.base.y - xy0.y;
    meter->needle.prev_xy[1].x=meter->needle.base.x - cos(angle*DEG_TO_RAD) * meter->needle.len;
    meter->needle.prev_xy[1].y=meter->needle.base.y - sin(angle*DEG_TO_RAD) * meter->needle.len;
    tft.drawLine(
        (float) meter->needle.prev_xy[0].x, 
        (float) meter->needle.prev_xy[0].y, 
        (float) meter->needle.prev_xy[1].x, 
        (float) meter->needle.prev_xy[1].y,         
        meter->needle.color);
    /*
    tft.drawLine( 
      meter->needle.prev_xy[0].x, 
      meter->needle.prev_xy[0].y, 
      meter->needle.prev_xy[1].x, 
      meter->needle.prev_xy[1].y, 
      meter->needle.color);
    */  
}

void draw_meter(meter_st *meter)
{
    char    value_str[12];
    draw_box(&meter->box);
    //tft.fillCircle(meter->needle.base.x, meter->needle.base.y, 6, meter->needle.color);
    
    // Draw ticks every 5 degrees from -50 to +50 degrees (100 deg. FSD swing)
    for (int i = meter->needle.range[0]; i <=  meter->needle.range[1]; i += 5) {
        // Long scale tick length
        int tl = 15;

        // Coodinates of tick to draw
        float sx = cos((i ) * DEG_TO_RAD);
        float sy = sin((i ) * DEG_TO_RAD);
        uint16_t x0 = -sx * (meter->needle.len + tl) + meter->box.mid.x;
        uint16_t y0 = -sy * (meter->needle.len + tl) + meter->needle.base.y;
        uint16_t x1 = -sx * meter->needle.len + meter->box.mid.x;
        uint16_t y1 = -sy * meter->needle.len + meter->needle.base.y;
        // Coordinates of next tick for zone fill
        float sx2 = cos((i + 5) * DEG_TO_RAD);
        float sy2 = sin((i + 5) * DEG_TO_RAD);
        int x2 = -sx2 * (meter->needle.len + tl) + meter->box.mid.x;
        int y2 = -sy2 * (meter->needle.len + tl) + meter->needle.base.y;
        int x3 = -sx2 * meter->needle.len + meter->box.mid.x;
        int y3 = -sy2 * meter->needle.len + meter->needle.base.y;
        
        //Serial.print("Full range =%d: " );
        //Serial.println( meter->needle.full_range );
        //Serial.printf("xyz: %d %d %d %d \n",x0,y0,x1,y1 );
        //Serial.printf("xyz: %d %d %d %d \n",x2,y2,x3,y3 );
        

        for (uint8_t sindx = 0; sindx < NBR_SECTIONS; sindx++)
        {
            if (( i >= meter->needle.section[sindx].range[0]) && 
               (i < meter->needle.section[sindx].range[1]))
            {
                tft.fillTriangle(x0, y0, x1, y1, x2, y2, meter->needle.section[sindx].color );
                tft.fillTriangle(x1, y1, x2, y2, x3, y3, meter->needle.section[sindx].color );
            }
        }

        if ((i - meter->needle.range[0]) % meter->needle.range[3] != 0) 
        {
            tl = 8;
        }  
        else
        {
            x0 = -sx * (meter->needle.len + tl*2) + meter->box.mid.x;
            y0 = -sy * (meter->needle.len + tl*2) + meter->needle.base.y;
            tft.setTextColor(TFT_BLACK);
            float v = ((float)(i-meter->needle.range[0]) *(meter->scale.max -meter->scale.min)) /(float) (meter->needle.range[1]-meter->needle.range[0]) + meter->scale.min;
            Serial.printf("%d - %d - %f\n",i, i % meter->needle.range[3], v);

            dtostrf(v, 4, 1, value_str);
            tft.drawCentreString(value_str, x0, y0, 1);           
        }
        // Recalculate coords incase tick lenght changed
        x0 = -sx * (meter->needle.len + tl) + meter->box.mid.x;
        y0 = -sy * (meter->needle.len + tl) + meter->needle.base.y;
        x1 = -sx * meter->needle.len + meter->box.mid.x;
        y1 = -sy * meter->needle.len + meter->needle.base.y;
       
        tft.drawLine(x0, y0, x1, y1, TFT_BLACK);  // Draw tick

    }


}

void draw_meter_value(meter_st *meter, float value)
{
    int16_t angle;
    char    value_str[12];
    static float   filter_val = 0;
    static uint16_t   iteration = 0;

        
    filter_val = (filter_val * 99.0 + value) / 100.0;
    if (++iteration > 9)
    {
        tft.fillRect(meter->box.reading_base.x, meter->box.reading_base.y, meter->box.reading_dim.x, meter->box.reading_dim.y,ILI9341_LIGHTGREY);
        tft.drawRect(meter->box.reading_base.x, meter->box.reading_base.y, meter->box.reading_dim.x, meter->box.reading_dim.y,meter->box.border);
        tft.setTextColor(TFT_BLACK);
        dtostrf(filter_val, 4, 2, value_str);
        tft.drawCentreString(value_str, meter->box.reading_text_mid.x , meter->box.reading_text_mid.y, 4); // Comment out to avoid font 4      
        iteration = 0;
    }

    angle = (value -meter->scale.min)/(meter->scale.max -meter->scale.min)*
        (meter->needle.range[1]-meter->needle.range[0]) + meter->needle.range[0];
    draw_needle(meter, angle);   

 
    

}

void loop() {

  float val;
  float dval = 0.1;
  val = meter.scale.min;
  
  draw_meter(&meter);
  while(1)
  {
      draw_meter_value(&meter,val);
      if ((val > meter.scale.max)||(val < meter.scale.min)) dval = -dval;
      val += dval;
    delay(10);
  }

  if (updateTime <= millis()) {
    updateTime = millis() + LOOP_PERIOD;

    d += 4; if (d >= 360) d = 0;

    //value[0] = map(analogRead(A0), 0, 1023, 0, 100); // Test with value form Analogue 0

    // Create a Sine wave for testing
    value[0] = 50 + 50 * sin((d + 0) * 0.0174532925);
    value[1] = 50 + 50 * sin((d + 60) * 0.0174532925);
    value[2] = 50 + 50 * sin((d + 120) * 0.0174532925);
    value[3] = 50 + 50 * sin((d + 180) * 0.0174532925);
    value[4] = 50 + 50 * sin((d + 240) * 0.0174532925);
    value[5] = 50 + 50 * sin((d + 300) * 0.0174532925);

    //unsigned long t = millis();

    //plotPointer();

    plotNeedle(value[0], 0);

    //Serial.println(millis()-t); // Print time taken for meter update
  }
}


// #########################################################################
//  Draw the analogue meter on the screen
// #########################################################################
void analogMeter()
{
  // Meter outline
  tft.fillRect(0, 0, 319, 160, TFT_GREY);
  tft.fillRect(5, 3, 309, 150, TFT_WHITE);

  tft.setTextColor(TFT_BLACK);  // Text colour

  // Draw ticks every 5 degrees from -50 to +50 degrees (100 deg. FSD swing)
  for (int i = -50; i < 51; i += 5) {
    // Long scale tick length
    int tl = 15;

    // Coodinates of tick to draw
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (100 + tl) + 160;
    uint16_t y0 = sy * (100 + tl) + 140;
    uint16_t x1 = sx * 100 + 160;
    uint16_t y1 = sy * 100 + 140;

    // Coordinates of next tick for zone fill
    float sx2 = cos((i + 5 - 90) * 0.0174532925);
    float sy2 = sin((i + 5 - 90) * 0.0174532925);
    int x2 = sx2 * (100 + tl) + 120;
    int y2 = sy2 * (100 + tl) + 140;
    int x3 = sx2 * 100 + 120;
    int y3 = sy2 * 100 + 140;

    
    // Green zone limits
    if (i >= 0 && i < 25) {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_GREEN);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_GREEN);
    }

    // Orange zone limits
    if (i >= 25 && i < 50) {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_ORANGE);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_ORANGE);
    }

    // Short scale tick length
    if (i % 25 != 0) tl = 8;

    // Recalculate coords incase tick lenght changed
    x0 = sx * (100 + tl) + 160;
    y0 = sy * (100 + tl) + 120;
    x1 = sx * 100 + 160;
    y1 = sy * 100 + 120;

    // Draw tick
    tft.drawLine(x0, y0, x1, y1, TFT_BLACK);

    // Check if labels should be drawn, with position tweaks
    if (i % 25 == 0) {
      // Calculate label positions
      x0 = sx * (100 + tl + 10) + 160;
      y0 = sy * (100 + tl + 10) + 120;
      switch (i / 25) {
        case -2: tft.drawCentreString("0", x0, y0 - 12, 2); break;
        case -1: tft.drawCentreString("25", x0, y0 - 9, 2); break;
        case 0: tft.drawCentreString("50", x0, y0 - 6, 2); break;
        case 1: tft.drawCentreString("75", x0, y0 - 9, 2); break;
        case 2: tft.drawCentreString("100", x0, y0 - 12, 2); break;
      }
    }

    // Now draw the arc of the scale
    sx = cos((i + 5 - 90) * 0.0174532925);
    sy = sin((i + 5 - 90) * 0.0174532925);
    x0 = sx * 100 + 160;
    y0 = sy * 100 + 120;
    // Draw scale arc, don't draw the last part
    if (i < 50) tft.drawLine(x0, y0, x1, y1, TFT_BLACK);
  }

  tft.drawString("%RH", 5 + 230 - 40, 119 - 20, 2); // Units at bottom right
  tft.drawCentreString("%RH", 120, 70, 4); // Comment out to avoid font 4
  tft.drawRect(5, 3, 320, 160, TFT_BLACK); // Draw bezel line

  plotNeedle(0, 0); // Put meter needle at 0
}

// #########################################################################
// Update needle position
// This function is blocking while needle moves, time depends on ms_delay
// 10ms minimises needle flicker if text is drawn within needle sweep area
// Smaller values OK if text not in sweep area, zero for instant movement but
// does not look realistic... (note: 100 increments for full scale deflection)
// #########################################################################
void plotNeedle(int value, byte ms_delay)
{
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  char buf[8]; dtostrf(value, 4, 0, buf);
  tft.drawRightString(buf, 40, 119 - 20, 2);

  if (value < -10) value = -10; // Limit value to emulate needle end stops
  if (value > 110) value = 110;

  // Move the needle util new value reached
  while (!(value == old_analog)) {
    if (old_analog < value) old_analog++;
    else old_analog--;

    if (ms_delay == 0) old_analog = value; // Update immediately id delay is 0

    float sdeg = map(old_analog, -10, 110, -150, -30); // Map value to angle
    // Calcualte tip of needle coords
    float sx = cos(sdeg * 0.0174532925);
    float sy = sin(sdeg * 0.0174532925);

    // Calculate x delta of needle start (does not start at pivot point)
    float tx = tan((sdeg + 90) * 0.0174532925);

    // Erase old needle image
    tft.drawLine(120 + 20 * ltx - 1, 140 - 20, osx - 1, osy, TFT_WHITE);
    tft.drawLine(120 + 20 * ltx, 140 - 20, osx, osy, TFT_WHITE);
    tft.drawLine(120 + 20 * ltx + 1, 140 - 20, osx + 1, osy, TFT_WHITE);

    // Re-plot text under needle
    tft.setTextColor(TFT_BLACK);
    tft.drawCentreString("%RH", 120, 70, 4); // // Comment out to avoid font 4

    // Store new needle end coords for next erase
    ltx = tx;
    osx = sx * 98 + 120;
    osy = sy * 98 + 140;

    // Draw the needle in the new postion, magenta makes needle a bit bolder
    // draws 3 lines to thicken needle
    tft.drawLine(120 + 20 * ltx - 1, 140 - 20, osx - 1, osy, TFT_RED);
    tft.drawLine(120 + 20 * ltx, 140 - 20, osx, osy, TFT_MAGENTA);
    tft.drawLine(120 + 20 * ltx + 1, 140 - 20, osx + 1, osy, TFT_RED);

    // Slow needle down slightly as it approaches new postion
    if (abs(old_analog - value) < 10) ms_delay += ms_delay / 5;

    // Wait before next update
    delay(ms_delay);
  }
}

// #########################################################################
//  Draw a linear meter on the screen
// #########################################################################
void plotLinear(char *label, int x, int y)
{
  int w = 36;
  tft.drawRect(x, y, w, 155, TFT_GREY);
  tft.fillRect(x + 2, y + 19, w - 3, 155 - 38, TFT_WHITE);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString(label, x + w / 2, y + 2, 2);

  for (int i = 0; i < 110; i += 10)
  {
    tft.drawFastHLine(x + 20, y + 27 + i, 6, TFT_BLACK);
  }

  for (int i = 0; i < 110; i += 50)
  {
    tft.drawFastHLine(x + 20, y + 27 + i, 9, TFT_BLACK);
  }

  tft.fillTriangle(x + 3, y + 127, x + 3 + 16, y + 127, x + 3, y + 127 - 5, TFT_RED);
  tft.fillTriangle(x + 3, y + 127, x + 3 + 16, y + 127, x + 3, y + 127 + 5, TFT_RED);

  tft.drawCentreString("---", x + w / 2, y + 155 - 18, 2);
}

// #########################################################################
//  Adjust 6 linear meter pointer positions
// #########################################################################
void plotPointer(void)
{
  int dy = 187;
  byte pw = 16;

  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  // Move the 6 pointers one pixel towards new value
  for (int i = 0; i < 6; i++)
  {
    char buf[8]; dtostrf(value[i], 4, 0, buf);
    tft.drawRightString(buf, i * 40 + 36 - 5, 187 - 27 + 155 - 18, 2);

    int dx = 3 + 40 * i;
    if (value[i] < 0) value[i] = 0; // Limit value to emulate needle end stops
    if (value[i] > 100) value[i] = 100;

    while (!(value[i] == old_value[i])) {
      dy = 187 + 100 - old_value[i];
      if (old_value[i] > value[i])
      {
        tft.drawLine(dx, dy - 5, dx + pw, dy, TFT_WHITE);
        old_value[i]--;
        tft.drawLine(dx, dy + 6, dx + pw, dy + 1, TFT_RED);
      }
      else
      {
        tft.drawLine(dx, dy + 5, dx + pw, dy, TFT_WHITE);
        old_value[i]++;
        tft.drawLine(dx, dy - 6, dx + pw, dy - 1, TFT_RED);
      }
    }
  }
}

