// --- Pin Definitions ---
#define POT_PIN   34
#define LDR_PIN   35
#define LED_PIN   4
#define NTC_PIN   32

// --- PWM Config ---
#define PWM_FREQ  5000
#define PWM_RES   8

// --- LDR Parameters (from Wokwi documentation) ---
// These constants MUST match the photoresistor's attributes in Wokwi [citation:4]
const float GAMMA = 0.7;   // Slope of log(R)/log(lux) graph
const float RL10 = 50.0;   // LDR resistance at 10 lux (in kilo-ohms)

// --- LDR Threshold (using raw ADC value, not Lux) ---
#define LDR_DARK_THRESHOLD 3500

// --- NTC Parameters ---
#define SERIES_RESISTOR 10000  // Series resistor value in ohms
#define NTC_NOMINAL     10000  // NTC resistance at 25°C
#define TEMPERATURE_NOMINAL 25 // Temperature for nominal resistance
#define B_COEFFICIENT   3950   // B coefficient of the thermistor

void setup() {
  Serial.begin(115200);
  ledcAttach(LED_PIN, PWM_FREQ, PWM_RES);
  analogReadResolution(12);  // ESP32 ADC resolution (0-4095)
}

void loop() {
  // --- Potentiometer ---
  int potVal = analogRead(POT_PIN);
  int brightness = map(potVal, 0, 4095, 0, 255);
  if (brightness > 0 && brightness < 30) brightness = 30;

  // --- LDR Read and Lux Calculation ---
  int ldrRaw = analogRead(LDR_PIN);
  float lux = calculateLux(ldrRaw);

  // Control LED based on raw LDR value (dark threshold)
  if (ldrRaw > LDR_DARK_THRESHOLD) {
    ledcWrite(LED_PIN, brightness);
  } else {
    ledcWrite(LED_PIN, 0);
  }

  // --- NTC Temperature Read ---
  int ntcRaw = analogRead(NTC_PIN);
  float temperature = readNTCtemperature(ntcRaw);

  // --- Serial Output (EXACT format you requested) ---
  Serial.print("Pot: ");
  Serial.print(potVal);
  Serial.print(" | LDR: ");
  Serial.print(ldrRaw);
  Serial.print(" | LDR lux: ");
  Serial.print(lux, 2);           // Lux with 2 decimal places
  Serial.print(" |NTC Temp ");
  Serial.print(temperature, 2);
  Serial.println(" °C");

  delay(1000);
}

// Function to convert LDR analog reading to Lux
float calculateLux(int analogValue) {
  // Step 1: Convert ADC reading (0-4095 for ESP32) to voltage (0-3.3V) [citation:5]
  float voltage = analogValue * (3.3 / 4095.0);
  
  // Step 2: Calculate LDR resistance using voltage divider formula [citation:4]
  // Circuit: VCC(3.3V) --- LDR --- AO pin --- 10K resistor --- GND
  // Voltage at AO pin = VCC * (R_fixed) / (R_ldr + R_fixed)
  // Rearranging: R_ldr = R_fixed * (VCC / voltage - 1)
  float resistance = 10000.0 * ((3.3 / voltage) - 1.0);
  
  // Step 3: Convert resistance to Lux using the formula [citation:2][citation:4]
  // Lux = (RL10 * 1000 * 10^GAMMA / R_ldr)^(1/GAMMA)
  float lux = pow(RL10 * 1000.0 * pow(10.0, GAMMA) / resistance, (1.0 / GAMMA));
  
  // Handle extreme cases (very bright = infinite lux)
  if (!isfinite(lux)) {
    return 999999.0;  // Return a very large number
  }
  
  return lux;
}

// Function to convert NTC analog reading to temperature in Celsius
float readNTCtemperature(int analogValue) {
  if (analogValue == 0) analogValue = 1;
  
  // Convert analog reading to resistance
  float resistance = SERIES_RESISTOR * (4095.0 / analogValue - 1.0);
  
  // Steinhart-Hart equation
  float steinhart;
  steinhart = resistance / NTC_NOMINAL;
  steinhart = log(steinhart);
  steinhart /= B_COEFFICIENT;
  steinhart += 1.0 / (TEMPERATURE_NOMINAL + 273.15);
  steinhart = 1.0 / steinhart;
  float temperatureC = steinhart - 273.15;
  
  return temperatureC;
}