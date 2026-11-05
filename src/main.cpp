#include <Arduino.h>
#include <WiFi.h>
#include <SD_MMC.h>
#include <NimBLEDevice.h>
#include <lvgl.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include "esp_lcd_st7735.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Official T-Dongle S3 pinout (non-Plus), active-LOW backlight.
static constexpr int BOOT=0, BACKLIGHT=38;
// T-Dongle S3 APA102 RGB LED: DATA GPIO40, CLOCK GPIO39 (BGR bytes).
static constexpr int LED_DATA=40, LED_CLOCK=39;
static uint32_t lastLedUpdate=0;
static uint32_t previousDeauth=0;
static uint32_t alertUntil=0;
static uint8_t ledBrightness=3; // 0..31: intentionally dim for USB operation

static void ledByte(uint8_t v){
  for(int bit=7;bit>=0;--bit){
    digitalWrite(LED_CLOCK,LOW);
    digitalWrite(LED_DATA,(v>>bit)&1);
    digitalWrite(LED_CLOCK,HIGH);
  }
}
static void ledRGB(uint8_t r,uint8_t g,uint8_t b){
  // APA102 start frame, one LED frame, and end frame.
  for(int i=0;i<4;i++)ledByte(0);
  ledByte(0xE0 | ledBrightness);
  ledByte(b);ledByte(g);ledByte(r);
  for(int i=0;i<4;i++)ledByte(0xFF);
}
static uint8_t triangle(uint32_t now,uint32_t period){
  uint32_t phase=now%period;
  uint32_t half=period/2;
  uint32_t value=phase<half?phase:period-phase;
  return (uint8_t)(25+(value*145)/half); // 25..170, no harsh flashing
}

static constexpr int SD_CLK=12, SD_CMD=16, SD_D0=14, SD_D1=17, SD_D2=21, SD_D3=18;
static esp_lcd_panel_handle_t panel=nullptr;
static esp_lcd_panel_io_handle_t panelIO=nullptr;
static uint8_t* drawBuffer=nullptr;
static lv_obj_t *titleLabel=nullptr, *bodyLabel=nullptr, *footLabel=nullptr;
static lv_display_t *displayInstance=nullptr;
static uint32_t lastUI=0, lastLog=0;

// The SPI LCD transfer is asynchronous. Do not free LVGL's buffer prematurely.
static bool onLCDTransferDone(esp_lcd_panel_io_handle_t,
                              esp_lcd_panel_io_event_data_t*, void*) {
  if(displayInstance) lv_display_flush_ready(displayInstance);
  return false;
}
static void flush(lv_display_t* d,const lv_area_t* a,uint8_t* colors) {
  lv_draw_sw_rgb565_swap(colors, lv_area_get_size(a));
  esp_err_t result=esp_lcd_panel_draw_bitmap(panel,a->x1,a->y1,a->x2+1,a->y2+1,colors);
  if(result!=ESP_OK){
    // No DMA completion event will follow on error; release LVGL here.
    lv_display_flush_ready(d);
  }
}
static uint32_t ticks(){return millis();}
static void setupChannelGraph();
static void initDisplay(){
  pinMode(BACKLIGHT,OUTPUT); digitalWrite(BACKLIGHT,HIGH);
  spi_bus_config_t bus=ST7735_PANEL_BUS_SPI_CONFIG(5,3,160*80*2);
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST,&bus,SPI_DMA_CH_AUTO));
  esp_lcd_panel_io_spi_config_t cfg=ST7735_PANEL_IO_SPI_CONFIG(4,2,onLCDTransferDone,nullptr);
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST,&cfg,&panelIO));
  esp_lcd_panel_dev_config_t dev={};
  dev.reset_gpio_num=1;
  dev.color_space=ESP_LCD_COLOR_SPACE_BGR;
  dev.bits_per_pixel=16;
  ESP_ERROR_CHECK(esp_lcd_new_panel_st7735(panelIO,&dev,&panel));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel,true));
  ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel,1,26));
  ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel,true));
  ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel,true,false));
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel,true));
  lv_init();
  // Full 160x80 RGB565 framebuffer. DMA-safe internal memory, not 16-line stripes.
  drawBuffer=(uint8_t*)heap_caps_malloc(160*80*2,MALLOC_CAP_DMA|MALLOC_CAP_INTERNAL);
  if(!drawBuffer){Serial.println("LVGL buffer allocation failed");abort();}
  lv_display_t* disp=lv_display_create(160,80);
  displayInstance=disp;
  lv_display_set_buffers(disp,drawBuffer,nullptr,160*80*2,LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_color_format(disp,LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(disp,flush);
  lv_tick_set_cb(ticks);
  lv_obj_t* root=lv_screen_active();
  lv_obj_set_style_bg_color(root,lv_color_black(),0);
  lv_obj_set_style_pad_all(root,0,0);
  titleLabel=lv_label_create(root);
  bodyLabel=lv_label_create(root);
  footLabel=lv_label_create(root);
  lv_obj_set_pos(titleLabel,8,3);
  lv_obj_set_pos(bodyLabel,8,28);
  lv_obj_set_size(titleLabel,144,21);
  lv_obj_set_size(bodyLabel,144,46);
  lv_label_set_long_mode(titleLabel,LV_LABEL_LONG_CLIP);
  lv_label_set_long_mode(bodyLabel,LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_font(titleLabel,&lv_font_montserrat_14,0);
  lv_obj_set_style_text_font(bodyLabel,&lv_font_montserrat_14,0);
  lv_obj_set_style_text_line_space(bodyLabel,2,0);
  lv_obj_set_style_text_color(titleLabel,lv_color_hex(0x00FFFF),0);
  lv_obj_set_style_text_color(bodyLabel,lv_color_white(),0);
  lv_obj_add_flag(footLabel,LV_OBJ_FLAG_HIDDEN); // no navigation help
  lv_label_set_text(titleLabel,"WesPen USB");
  setupChannelGraph();
  lv_label_set_text(bodyLabel,"Display online");
  lv_timer_handler();
  digitalWrite(BACKLIGHT,LOW);
}

// PCAP: LINKTYPE_IEEE802_11 = 105. Only selected receive frames, no transmission.
struct Frame {uint32_t sec,usec; uint16_t len; uint8_t bytes[512];};
static QueueHandle_t frameQueue=nullptr;
static File pcap;
static bool sdOK=false, monitorOn=false;
static volatile uint32_t rxFrames=0,eapolFrames=0,deauthFrames=0,disassocFrames=0,beacons=0,dropped=0;
static uint32_t savedFrames=0;
static int channel=1,networks=0,bleCount=0;
static int counts[14]={0};
// WESPEN_TRAFFIC_V1: observed frame traffic per channel (relative, not CCA airtime).
// Only decodable 802.11 RX frames are counted. Cannot measure non-Wi-Fi interference.
static constexpr uint32_t CH_DWELL_MS=300;
static uint32_t trafficBytes[14]={0};
// WESPEN_ADVICE_V1: comparisons from sampled RX byte totals (not measured airtime).
static uint32_t trafficSamples=0;
static bool trafficHasSweep=false;
static bool trafficSamplesForCh[14]={false};
static void channelAdvice(char* dest,size_t destLen){
  if(!trafficHasSweep){snprintf(dest,destLen,"Scanning...\nCH 1-13");return;}
  // 1..11 are typically available; compare only sampled channels.
  int quiet=0, busy=0, best=0;
  for(int c=1;c<=13;c++){
    if(!trafficSamplesForCh[c])continue;
    if(!quiet || trafficBytes[c]<trafficBytes[quiet])quiet=c;
    if(!busy || trafficBytes[c]>trafficBytes[busy])busy=c;
  }
  const int nonOverlap[3]={1,6,11};
  for(int c:nonOverlap){
    if(trafficSamplesForCh[c] && (!best || trafficBytes[c]<trafficBytes[best]))best=c;
  }
  if(quiet && busy && best)
    snprintf(dest,destLen,"Quiet:%d Busy:%d\nBest 1/6/11:%d",quiet,busy,best);
  else snprintf(dest,destLen,"Scanning...\nNo valid data");
}

static uint32_t trafficPackets[14]={0};
static volatile uint32_t dwellBytes=0, dwellPackets=0;
static bool trafficActive=false;
static int trafficChannel=1;
static uint32_t dwellStarted=0;
static void onTrafficRx(void* raw,wifi_promiscuous_pkt_type_t kind){
  if(!raw || kind==WIFI_PKT_MISC)return;
  const auto* pkt=static_cast<const wifi_promiscuous_pkt_t*>(raw);
  const uint32_t len=pkt->rx_ctrl.sig_len;
  if(len==0 || len>4096)return;
  ++dwellPackets;
  dwellBytes+=len;
}
static void stopTraffic(){
  if(!trafficActive)return;
  esp_wifi_set_promiscuous(false);
  trafficActive=false;
  esp_wifi_set_promiscuous_rx_cb(nullptr);
}
static void startTraffic(){
  stopTraffic();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false,true);
  memset(trafficBytes,0,sizeof(trafficBytes));
  memset(trafficPackets,0,sizeof(trafficPackets));
  memset(trafficSamplesForCh,0,sizeof(trafficSamplesForCh));
  trafficSamples=0;trafficHasSweep=false;
  trafficChannel=1;
  dwellBytes=0;dwellPackets=0;
  wifi_promiscuous_filter_t filter={};
  filter.filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT|WIFI_PROMIS_FILTER_MASK_DATA|WIFI_PROMIS_FILTER_MASK_CTRL;
  if(esp_wifi_set_promiscuous_filter(&filter)!=ESP_OK)return;
  if(esp_wifi_set_promiscuous_rx_cb(onTrafficRx)!=ESP_OK)return;
  // If a channel is unsupported by the local radio configuration, leave its bar empty.
  if(esp_wifi_set_channel(trafficChannel,WIFI_SECOND_CHAN_NONE)!=ESP_OK)return;
  if(esp_wifi_set_promiscuous(true)!=ESP_OK)return;
  trafficActive=true;
  dwellStarted=millis();
}
static void pollTraffic(){
  if(!trafficActive || millis()-dwellStarted<CH_DWELL_MS)return;
  esp_wifi_set_promiscuous(false);
  trafficBytes[trafficChannel]=dwellBytes;
  trafficSamplesForCh[trafficChannel]=true;
  if(++trafficSamples>=13)trafficHasSweep=true;
  trafficPackets[trafficChannel]=dwellPackets;
  trafficChannel=(trafficChannel%13)+1;
  dwellBytes=0;dwellPackets=0;
  // Try at most 13 channels to avoid looping indefinitely on unsupported channels.
  for(int attempt=0;attempt<13;attempt++){
    if(esp_wifi_set_channel(trafficChannel,WIFI_SECOND_CHAN_NONE)==ESP_OK){
      esp_wifi_set_promiscuous(true);
      dwellStarted=millis();
      return;
    }
    trafficBytes[trafficChannel]=0;
    trafficPackets[trafficChannel]=0;
    trafficChannel=(trafficChannel%13)+1;
  }
  stopTraffic();
}

// WESPEN_GRAPH_V1: LVGL mini bar graph, channels 1 through 13.
static lv_obj_t* channelBars[13]={nullptr};
static const int CH_GRAPH_X=8, CH_GRAPH_BOTTOM=79;
static constexpr int CH_BAR_W=8, CH_BAR_STEP=11, CH_GRAPH_MAX_H=20;
static void setupChannelGraph(){
  lv_obj_t* root=lv_screen_active();
  for(int i=0;i<13;i++){
    channelBars[i]=lv_obj_create(root);
    lv_obj_remove_style_all(channelBars[i]);
    lv_obj_set_style_bg_opa(channelBars[i],LV_OPA_COVER,0);
    lv_obj_set_style_bg_color(channelBars[i],
      lv_color_hex((i==0||i==5||i==10)?0x00FFAA:0x26A8FF),0);
    lv_obj_set_pos(channelBars[i],CH_GRAPH_X+i*CH_BAR_STEP,CH_GRAPH_BOTTOM-1);
    lv_obj_set_size(channelBars[i],CH_BAR_W,1);
    lv_obj_add_flag(channelBars[i],LV_OBJ_FLAG_HIDDEN);
  }
}
static void showChannelGraph(bool visible){
  for(int i=0;i<13;i++)if(channelBars[i]){
    if(visible)lv_obj_remove_flag(channelBars[i],LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(channelBars[i],LV_OBJ_FLAG_HIDDEN);
  }
}
static void updateChannelGraph(){
  // Each bar compares observed receive byte volume per equal 300ms sample.
  // The scale re-normalizes to the busiest sampled channel, NOT 0-100% airtime.
  uint32_t peak=1;
  for(int ch=1;ch<=13;ch++){
    if(trafficBytes[ch]>peak)peak=trafficBytes[ch];
  }
  for(int i=0;i<13;i++){
    uint32_t amount=trafficBytes[i+1];
    int h=amount?max(2,(int)((uint64_t)amount*CH_GRAPH_MAX_H/peak)):1;
    lv_obj_set_pos(channelBars[i],CH_GRAPH_X+i*CH_BAR_STEP,CH_GRAPH_BOTTOM-h);
    lv_obj_set_size(channelBars[i],CH_BAR_W,h);
    lv_obj_set_style_bg_color(channelBars[i],lv_color_hex(
      h>CH_GRAPH_MAX_H*2/3?0xFF4242:(h>CH_GRAPH_MAX_H/3?0xFFB020:0x30D774)),0);
  }
}

static String strongest="-";
// Cache scan results so the tiny display can show a moving list.
static constexpr int MAX_SCAN_ITEMS=40;
struct ScanItem {String name; int rssi; int channel;};
static ScanItem wifiItems[MAX_SCAN_ITEMS];
static ScanItem bleItems[MAX_SCAN_ITEMS];
static int wifiItemCount=0,bleItemCount=0;
static int listOffset=0;
static uint32_t lastListMove=0,lastScan=0;
static constexpr uint32_t LIST_ADVANCE_MS=2300;
static constexpr uint32_t WIFI_RESCAN_MS=20000;
static constexpr uint32_t BLE_RESCAN_MS=20000;

static String compactLabel(const String& value){
  String result=value;
  result.replace("\n"," ");result.replace("\r"," ");
  if(result.length()>13)result=result.substring(0,13);
  return result;
}
static bool captureOpen=false;

static void write32(File& f,uint32_t v){uint8_t b[4]={(uint8_t)v,(uint8_t)(v>>8),(uint8_t)(v>>16),(uint8_t)(v>>24)};f.write(b,4);}
static void write16(File& f,uint16_t v){uint8_t b[2]={(uint8_t)v,(uint8_t)(v>>8)};f.write(b,2);}
static void closeCapture(){if(captureOpen){pcap.flush();pcap.close();captureOpen=false;}}
static void openCapture(){
  closeCapture(); savedFrames=0;
  if(!sdOK)return;
  for(int i=0;i<100;i++){
    String path="/CAP"+String(i)+".pcap";
    if(SD_MMC.exists(path))continue;
    pcap=SD_MMC.open(path,FILE_WRITE);
    if(!pcap)return;
    write32(pcap,0xa1b2c3d4);write16(pcap,2);write16(pcap,4);
    write32(pcap,0);write32(pcap,0);write32(pcap,512);write32(pcap,105);
    captureOpen=true;Serial.printf("PCAP: %s\n",path.c_str());return;
  }
  Serial.println("PCAP filenames CAP0-CAP99 exhausted");
}
static void drainQueue(){
  if(!frameQueue)return;
  Frame f;
  int batch=0;
  while(batch++<16 && xQueueReceive(frameQueue,&f,0)==pdTRUE){
    if(!captureOpen)continue;
    write32(pcap,f.sec);write32(pcap,f.usec);write32(pcap,f.len);write32(pcap,f.len);
    pcap.write(f.bytes,f.len);savedFrames++;
  }
  if(captureOpen && millis()-lastLog>2000){pcap.flush();lastLog=millis();}
}
static void onRx(void* raw,wifi_promiscuous_pkt_type_t type){
  if(!raw||type==WIFI_PKT_MISC)return;
  auto* p=(wifi_promiscuous_pkt_t*)raw;
  int n=p->rx_ctrl.sig_len;
  if(n<24 || n>2304)return;
  const uint8_t* b=p->payload;
  int frameType=(b[0]>>2)&3, sub=(b[0]>>4)&15;
  bool beacon=(frameType==0 && sub==8);
  bool deauth=(frameType==0 && sub==12);
  bool disassoc=(frameType==0 && sub==10);
  bool eapol=false;
  if(frameType==2 && !(b[1]&0x40)){
    int hdr=24+(((b[1]&3)==3)?6:0)+((sub&8)?2:0);
    if((sub&8)&&(b[1]&0x80))hdr+=4;
    if(n>=hdr+8 && b[hdr]==0xaa && b[hdr+1]==0xaa && b[hdr+2]==3 &&
       b[hdr+6]==0x88 && b[hdr+7]==0x8e)eapol=true;
  }
  ++rxFrames;
  if(beacon)++beacons;
  if(deauth)++deauthFrames;
  if(disassoc)++disassocFrames;
  if(eapol)++eapolFrames;
  // Beacons include SSID/context; EAPOL provides key exchange, without payload encryption bypass.
  if(!(beacon||deauth||disassoc||eapol)||!frameQueue)return;
  Frame f={};
  f.len=min(n,(int)sizeof(f.bytes));
  uint64_t us=esp_timer_get_time(); f.sec=us/1000000;f.usec=us%1000000;
  memcpy(f.bytes,b,f.len);
  if(xQueueSend(frameQueue,&f,0)!=pdTRUE)++dropped;
}
static void stopMonitor(){
  if(!monitorOn)return;
  esp_wifi_set_promiscuous(false);
  monitorOn=false;
  drainQueue();
  closeCapture();
}
static void startMonitor(){
  WiFi.mode(WIFI_STA);WiFi.disconnect(false,true);
  wifi_promiscuous_filter_t f={};f.filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT|WIFI_PROMIS_FILTER_MASK_DATA;
  esp_wifi_set_promiscuous_filter(&f);
  esp_wifi_set_promiscuous_rx_cb(onRx);
  openCapture();
  if(esp_wifi_set_channel(channel,WIFI_SECOND_CHAN_NONE)==ESP_OK && esp_wifi_set_promiscuous(true)==ESP_OK)monitorOn=true;
  else closeCapture();
}
static void scanWiFi(){
  stopMonitor();
  WiFi.mode(WIFI_STA);WiFi.disconnect(false,true);
  int result=WiFi.scanNetworks(false,true,false,140);
  networks=result<0?0:result;
  memset(counts,0,sizeof(counts));strongest="-";
  wifiItemCount=0;int strongestRSSI=-999;
  for(int i=0;i<networks;i++){
    int c=WiFi.channel(i);
    if(c>=1&&c<=13)counts[c]++;
    int rssi=WiFi.RSSI(i);
    String name=WiFi.SSID(i);
    if(name.isEmpty())name="<hidden>";
    if(rssi>strongestRSSI){strongestRSSI=rssi;strongest=name;}
    if(wifiItemCount<MAX_SCAN_ITEMS){
      wifiItems[wifiItemCount++]={compactLabel(name),rssi,c};
    }
  }
  // Strongest first, so useful networks appear first on the tiny screen.
  for(int i=0;i<wifiItemCount;i++)for(int j=i+1;j<wifiItemCount;j++){
    if(wifiItems[j].rssi>wifiItems[i].rssi){
      ScanItem temp=wifiItems[i];wifiItems[i]=wifiItems[j];wifiItems[j]=temp;
    }
  }
  if(strongest.length()>16)strongest=strongest.substring(0,16);
  WiFi.scanDelete();
  listOffset=0;lastListMove=millis();lastScan=millis();
}
static void scanBLE(){
  stopMonitor();
  static bool initialized=false;
  if(!initialized){NimBLEDevice::init("");initialized=true;}
  NimBLEScan* scanner=NimBLEDevice::getScan();
  scanner->setActiveScan(true); // Requests scan-response names where available.
  scanner->setInterval(120);scanner->setWindow(60);
  NimBLEScanResults results=scanner->start(3,false);
  bleCount=results.getCount();bleItemCount=0;
  for(int i=0;i<bleCount && bleItemCount<MAX_SCAN_ITEMS;i++){
    NimBLEAdvertisedDevice dev=results.getDevice(i);
    String name=String(dev.getName().c_str());
    if(name.isEmpty()){
      String mac=String(dev.getAddress().toString().c_str());
      name="BLE " + mac.substring(mac.length()>8?mac.length()-8:0);
    }
    bleItems[bleItemCount++]={compactLabel(name),dev.getRSSI(),0};
  }
  scanner->clearResults();
  for(int i=0;i<bleItemCount;i++)for(int j=i+1;j<bleItemCount;j++){
    if(bleItems[j].rssi>bleItems[i].rssi){
      ScanItem temp=bleItems[i];bleItems[i]=bleItems[j];bleItems[j]=temp;
    }
  }
  listOffset=0;lastListMove=millis();lastScan=millis();
}
static void drawScanList(const ScanItem* items,int count){
  if(count<=0){lv_label_set_text(bodyLabel,"No devices found");return;}
  // 160x80 LCD, 14 px font: two visible rows below the title.
  char b[112];
  const ScanItem& a=items[listOffset%count];
  if(count==1){
    snprintf(b,sizeof(b),"%s %ddBm",a.name.c_str(),a.rssi);
  } else {
    const ScanItem& z=items[(listOffset+1)%count];
    snprintf(b,sizeof(b),"%s %ddBm\n%s %ddBm",a.name.c_str(),a.rssi,z.name.c_str(),z.rssi);
  }
  lv_label_set_text(bodyLabel,b);
}
enum Page {MENU,WIFI_PAGE,CHANNELS,MONITOR,BLE_PAGE,STORAGE,EAPOL_PAGE,DEAUTH_PAGE,SELECT_AP};
static Page page=MENU;
static const char* names[]={"WiFi scanner","Channel analyzer","Packet monitor","BLE scanner","MicroSD / PCAP","EAPOL monitor","Deauth detector"};
static int selected=0;
static int selectedAP=0;
static bool isMonitorPage(Page p){
  return p==MONITOR || p==EAPOL_PAGE || p==DEAUTH_PAGE;
}
// Each time a monitor page is entered or exited, start with a fresh session.
// Close the capture first so queued frames from the previous session cannot
// contribute to counters or the next PCAP file.
static void resetMonitorCounters(){
  rxFrames=0;
  eapolFrames=0;
  deauthFrames=0;
  disassocFrames=0;
  beacons=0;
  dropped=0;
  savedFrames=0;
  previousDeauth=0;
  alertUntil=0;
  lastLog=millis();
}
static void enter(Page p){
  stopTraffic();
  Page oldPage=page;
  stopMonitor();
  if(isMonitorPage(oldPage) || isMonitorPage(p))resetMonitorCounters();
  page=p;
  showChannelGraph(p==CHANNELS);
  if(p==WIFI_PAGE || p==SELECT_AP) {
    scanWiFi();
    if(p==SELECT_AP) selectedAP=0;
  }
  if(p==CHANNELS)startTraffic();
  if(p==BLE_PAGE)scanBLE();
  if(isMonitorPage(p))startMonitor();
}
static void draw(){
  char b[110];
  if(page==CHANNELS){
    lv_label_set_text(titleLabel,"Traffic / CH");
    channelAdvice(b,sizeof(b));
    lv_label_set_text(bodyLabel,b);
    // Reserve the lower part of the LCD for the 13-channel graph.
    lv_obj_set_pos(bodyLabel,8,24);
    lv_obj_set_size(bodyLabel,144,34);
    updateChannelGraph();
    return;
  }
  lv_obj_set_pos(bodyLabel,8,28);
  lv_obj_set_size(bodyLabel,144,46);
  if(page==MENU){
    lv_label_set_text(titleLabel,"WesPen USB");
    snprintf(b,sizeof(b),"%d/7\n%s",selected+1,names[selected]);
    lv_label_set_text(bodyLabel,b);
    return;
  }
  if(page==SELECT_AP){
    lv_label_set_text(titleLabel,"Choose WiFi AP");
    if(selectedAP>=wifiItemCount){
      lv_label_set_text(bodyLabel,"< Back to menu");
    }else{
      const ScanItem& ap=wifiItems[selectedAP];
      snprintf(b,sizeof(b),"%d/%d %s\nCH %d  %d dBm",selectedAP+1,wifiItemCount,ap.name.c_str(),ap.channel,ap.rssi);
      lv_label_set_text(bodyLabel,b);
    }
    return;
  }
  if(page==WIFI_PAGE){
    snprintf(b,sizeof(b),"WiFi  %d APs",networks);
    lv_label_set_text(titleLabel,b);
  }else if(page==BLE_PAGE){
    snprintf(b,sizeof(b),"BLE  %d devices",bleCount);
    lv_label_set_text(titleLabel,b);
  }else lv_label_set_text(titleLabel,names[(int)page-1]);
  switch(page){
    case WIFI_PAGE:drawScanList(wifiItems,wifiItemCount);return;
    case CHANNELS:snprintf(b,sizeof(b),"APs: %d\nCH 1/6/11: %d/%d/%d",networks,counts[1],counts[6],counts[11]);break;
    case MONITOR:snprintf(b,sizeof(b),"CH %d  RX: %lu\nSaved: %lu  Drop: %lu",channel,(unsigned long)rxFrames,(unsigned long)savedFrames,(unsigned long)dropped);break;
    case BLE_PAGE:drawScanList(bleItems,bleItemCount);return;
    case STORAGE:snprintf(b,sizeof(b),"SD: %s\nCaptures: /CAP*.pcap",sdOK?"OK":"NOT MOUNTED");break;
    case EAPOL_PAGE:snprintf(b,sizeof(b),"CH%d EAPOL:%lu\nDeauth:%lu Dis:%lu",channel,(unsigned long)eapolFrames,(unsigned long)deauthFrames,(unsigned long)disassocFrames);break;
    case DEAUTH_PAGE:snprintf(b,sizeof(b),"Deauth: %lu\nDisassoc: %lu",(unsigned long)deauthFrames,(unsigned long)disassocFrames);break;
    default:b[0]=0;break;
  }
  lv_label_set_text(bodyLabel,b);
}
static void tap(){
  if(page==MENU)selected=(selected+1)%7;
  else if(page==WIFI_PAGE){if(wifiItemCount)listOffset=(listOffset+1)%wifiItemCount;lastListMove=millis();}
  else if(page==CHANNELS)startTraffic();
  else if(page==SELECT_AP)selectedAP=(selectedAP+1)%(wifiItemCount+1);
  else if(page==BLE_PAGE){if(bleItemCount)listOffset=(listOffset+1)%bleItemCount;lastListMove=millis();}
  else if(page==STORAGE){SD_MMC.end();sdOK=SD_MMC.begin("/sdcard",false);}
  else {stopMonitor();channel=channel==11?1:channel+1;startMonitor();}
  draw();
}
static void hold(){
  if(page==MENU){
    // The EAPOL workflow now asks for an access point first.
    if(selected==5)enter(SELECT_AP);
    else enter((Page)(selected+1));
  }else if(page==SELECT_AP){
    if(selectedAP>=wifiItemCount)enter(MENU);
    else{
      channel=wifiItems[selectedAP].channel;
      if(channel<1 || channel>13)channel=1;
      enter(EAPOL_PAGE);
    }
  }else enter(MENU);
  draw();
}
static void updateLed(uint32_t now){
  if(now-lastLedUpdate<70)return;
  lastLedUpdate=now;
  uint32_t seen=deauthFrames+disassocFrames;
  if(seen!=previousDeauth){
    previousDeauth=seen;
    alertUntil=now+1600;
  }
  uint8_t wave=triangle(now,1800);
  if((page==DEAUTH_PAGE || page==EAPOL_PAGE || page==MONITOR) &&
     (int32_t)(alertUntil-now)>0){
    // Distinct 150 ms red flash on/off; each new event extends the alert.
    if(((now/150)%2)==0)ledRGB(255,0,0);
    else ledRGB(0,0,0);
  }else if(page==MENU){
    // Slow color cycle: cyan -> purple -> green, never flashing sharply.
    uint32_t stage=(now/2400)%3;
    if(stage==0)ledRGB(0,wave,wave);
    else if(stage==1)ledRGB(wave/2,0,wave);
    else ledRGB(0,wave,wave/3);
  }else if(page==WIFI_PAGE || page==CHANNELS){
    ledRGB(0,wave,wave);          // cyan: WiFi discovery
  }else if(page==BLE_PAGE){
    ledRGB(wave/2,0,wave);        // purple: BLE discovery
  }else if(page==STORAGE){
    if(sdOK)ledRGB(0,wave,0);    // green: SD mounted
    else ledRGB(wave/2,0,0);     // subdued red: SD missing
  }else if(page==EAPOL_PAGE){
    ledRGB(wave,wave/2,0);        // amber: EAPOL receive mode
  }else if(page==DEAUTH_PAGE){
    ledRGB(0,wave/3,wave/2);      // calm blue when no alert
  }else{
    ledRGB(0,wave,0);           // green: packet monitor
  }
}
static void pollBoot(){
  static bool prev=false;static uint32_t since=0,changed=0;
  const uint32_t now=millis();bool down=digitalRead(BOOT)==LOW;
  if(down!=prev && now-changed>35){
    changed=now;prev=down;
    if(down)since=now;
    else if(now-since>=650)hold();
    else if(now-since>=35)tap();
  }
}
void setup(){
  Serial.begin(115200);pinMode(BOOT,INPUT_PULLUP);
  pinMode(LED_DATA,OUTPUT);pinMode(LED_CLOCK,OUTPUT);
  digitalWrite(LED_DATA,LOW);digitalWrite(LED_CLOCK,LOW);
  ledRGB(0,50,50);
  initDisplay();
  Serial.println("ST7735 display initialized (LILYGO driver)");
  SD_MMC.setPins(SD_CLK,SD_CMD,SD_D0,SD_D1,SD_D2,SD_D3);
  sdOK=SD_MMC.begin("/sdcard",false);
  Serial.printf("SD: %s\n",sdOK?"ready":"missing");
  frameQueue=xQueueCreate(12,sizeof(Frame));
  if(!frameQueue)Serial.println("Queue allocation failed - PCAP disabled");
  WiFi.mode(WIFI_STA);draw();
}
void loop(){
  pollBoot();pollTraffic();drainQueue();
  uint32_t now=millis();
  if(page==WIFI_PAGE || page==BLE_PAGE){
    const int count=page==WIFI_PAGE?wifiItemCount:bleItemCount;
    if(count>1 && now-lastListMove>=LIST_ADVANCE_MS){
      listOffset=(listOffset+1)%count;
      lastListMove=now;
    }
    const uint32_t interval=page==WIFI_PAGE?WIFI_RESCAN_MS:BLE_RESCAN_MS;
    if(now-lastScan>=interval){
      if(page==WIFI_PAGE)scanWiFi();else scanBLE();
      now=millis();
    }
  }
  if(now-lastUI>300){lastUI=now;draw();}
  updateLed(millis());
  lv_timer_handler();delay(5);
}
