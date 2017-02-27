#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_date_layer;
static TextLayer *s_time_layer;

/*
  * Calendar layers 
  * Example: 27.02.2017
    | 0  1  2  3  4  5  6  7
    |-------------------------
   0|Mo  Tu We Th Fr Sa So CW
   1|27  28 1  2  3  4  5  9
   2|6   7  8  9  10 11 12 10
   3|13  14 15 16 17 18 19 11
*/
static TextLayer *s_cal_layer[4][8]; 
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

char *itoa(int num)
{
  static char buff[20] = {};
  int i = 0, temp_num = num, length = 0;
  char *string = buff;
  if(num >= 0) {
    // count how many characters in the number
    while(temp_num) {
      temp_num /= 10;
      length++;
  }
  // assign the number to the buffer starting at the end of the
  // number and going to the begining since we are doing the
  // integer to character conversion on the last number in the
  // sequence
  for(i = 0; i < length; i++) {
    buff[(length-1)-i] = '0' + (num % 10);
    num /= 10;  
  }
  buff[i] = '\0'; // can't forget the null byte to properly end our string
  }
  else
    return "Unsupported Number";
  return string;
}

int month_length(struct tm * ptm){
  int mon_len = ptm -> tm_mday;//init with day of month
  int month = ptm -> tm_mon;// init with current month
  time_t next = mktime(ptm); //seconds timestamp of the current day in UTC
  struct tm * nextday;
  
  while(mon_len <= 31){
    next = next + SECONDS_PER_DAY; //next day timestamp
    nextday = localtime(&next);
    if (nextday->tm_mon == month){ 
      mon_len++;
    }
    else{
      break;
    }
  }
  
  return mon_len;
}


static void update_calendar(){
  //get week day from tm-struct(0-Sunday, 6-Saturday)
  //convert to 0-Monday, 6-Sunday format
  
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  int i; 
  int weekday = tick_time->tm_wday == 0 ? 6 : (tick_time->tm_wday - 1);
  int firstday = tick_time->tm_mday - weekday;//Begin of the week
  int day = firstday;//current day
  /*
   * Buffer for calendar variables
   * Example: 27.02.2017
     | 0  1  2  3  4  5  6  7
     |-------------------------
    0|Mo  Tu We Th Fr Sa So CW
    1|27  28 1  2  3  4  5  9
    2|6   7  8  9  10 11 12 10
    3|13  14 15 16 17 18 19 11
  */
  static char s_buffer[4][8][3];
  //current row in calendar:0 - name of the day, 
  //                        1 - current week, 
  //                        2 - next week, 
  //                        3 - week after next  
  int row =0;
  
  int current_week;
  int mon_len = month_length(tick_time);
  
  //find out current week
  strftime(s_buffer[1][7], sizeof(s_buffer[1][7]), "%2W", tick_time);
  //get tensdigit and onesdigit from stringbuffer, substruct from ASCII '0'
  //to get number
  current_week = (s_buffer[1][7][0] - '0')*10 + (s_buffer[1][7][1] - '0');
  
  //fill the buffer
  //0 row: Name of the days + CW
  strncpy(s_buffer[row][0],"Mo",2);
  strncpy(s_buffer[row][1],"Tu",2);
  strncpy(s_buffer[row][2],"We",2);
  strncpy(s_buffer[row][3],"Th",2);
  strncpy(s_buffer[row][4],"Fr",2);
  strncpy(s_buffer[row][5],"Sa",2);
  strncpy(s_buffer[row][6],"Su",2);
  strncpy(s_buffer[row][7],"CW",2);
  
  //1-3 rows: Days of the month: current week, next and week after next
  for (row=1;row<4;row++){
    //days of the week
    for (i=0;i<7;i++){
      strncpy(s_buffer[row][i], itoa(day), 2);
      if(day >= mon_len){
        day = 1;
      }
      else{
        day++;
      }    
    }
    //calendar week 
    strncpy(s_buffer[row][7], itoa(current_week + (row-1)), 2);
  }
  
  
  //update week days line (mark current day in the line)
  row = 0;
  for (i=0;i<8;i++){
    text_layer_set_text(s_cal_layer[row][i], s_buffer[row][i]); 
  }
  
  //update current week line
  row = 1;
  for (i=0;i<8;i++){
    if (i == weekday || i == 7){//current day or current week
      text_layer_set_background_color(s_cal_layer[row][i], GColorWhite);
      text_layer_set_text_color(s_cal_layer[row][i], GColorBlack);
    }
    else {
      text_layer_set_background_color(s_cal_layer[row][i], GColorBlack);
      text_layer_set_text_color(s_cal_layer[row][i], GColorWhite);   
    }
    text_layer_set_text(s_cal_layer[row][i], s_buffer[row][i]); 
  }
  //next week and week after
  for (row=2;row<4;row++){
    for (i=0;i<8;i++){
      text_layer_set_text(s_cal_layer[row][i], s_buffer[row][i]); 
    }
  }
  
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  
  //Write the current date into a buffer
  static char s_date_buffer[14];
  strftime(s_date_buffer, sizeof(s_date_buffer), " %B %d", tick_time);

  // Display this date on the TextLayer
  text_layer_set_text(s_date_layer, s_date_buffer);
  
}


static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void day_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_calendar();
}

static void main_window_load(Window *window) {
  
  int i,j;
  int width;
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  tick_timer_service_subscribe(DAY_UNIT, day_handler);

  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  //Set Background Color
  window_set_background_color(window, GColorBlack);
  
  //------------DATE-------------------------------------------------------
  s_date_layer = text_layer_create(
      GRect(0, 10, bounds.size.w, 30));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  //----------------------------------------------
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  //-----------------------------------------------------------------------
  
  
  //------------------- TIME -----------------------------------------------
  s_time_layer = text_layer_create( GRect(0, 35, bounds.size.w, 45));

  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  //---------------------------------------------------------------------------
  
  //-------------------------- Calendar ----------------------------------------
  width = (int) bounds.size.w/8.0;
  for (i=0; i<4; i++){
    for (j=0; j<8; j++){
      s_cal_layer[i][j] = text_layer_create(GRect(j*width, 80 + i*20, width, 20));
      text_layer_set_background_color(s_cal_layer[i][j], GColorClear);
      text_layer_set_text_color(s_cal_layer[i][j], GColorWhite);
      text_layer_set_font(s_cal_layer[i][j], fonts_get_system_font(FONT_KEY_GOTHIC_14));
      text_layer_set_text_alignment(s_cal_layer[i][j], GTextAlignmentCenter);
      layer_add_child(window_layer, text_layer_get_layer(s_cal_layer[i][j]));
    }
  }
  //-------------------------------------------------------------------------------
  
  
}

static void main_window_unload(Window *window) {
  int i,j;
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  // Destroy Date TextLayer
  text_layer_destroy(s_date_layer);
  for (i=0;i<4;i++){
    for (j=0;j<8;j++){
      text_layer_destroy(s_cal_layer[i][j]);
    }
  }
  // Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);

  // Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Make sure the time is displayed from the start
  update_time();
  update_calendar();

}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
