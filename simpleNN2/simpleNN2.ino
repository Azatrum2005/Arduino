#include "model_data.h"  // Your converted model header
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

// TensorFlow Lite globals
namespace {
  tflite::ErrorReporter* error_reporter = nullptr;
  const tflite::Model* model = nullptr;
  tflite::MicroInterpreter* interpreter = nullptr;
  TfLiteTensor* input = nullptr;
  TfLiteTensor* output = nullptr;
  
  // Quantization parameters
  float input_scale = 0.0f;
  int32_t input_zero_point = 0;
  float output_scale = 0.0f;
  int32_t output_zero_point = 0;
  
  // Memory arena
  constexpr int kTensorArenaSize = 30 * 1024;
  alignas(16) uint8_t tensor_arena[kTensorArenaSize];
}

// Serial input buffer
String inputString = "";
bool stringComplete = false;

// Quantize float to int8
int8_t quantize(float value, float scale, int32_t zero_point) {
  int32_t quantized = round(value / scale) + zero_point;
  if(quantized < -128) quantized = -128;
  if(quantized > 127) quantized = 127;
  return (int8_t)quantized;
}

// Dequantize int8 to float
float dequantize(int8_t value, float scale, int32_t zero_point) {
  return (float)(value - zero_point) * scale;
}

// Initialize TensorFlow Lite
bool initializeTFLite() {
  Serial.println("\n=== Initializing TensorFlow Lite ===");
  
  // Set up error reporter
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;
  
  // Load model from flash memory (embedded in firmware)
  model = tflite::GetModel(model_tflite);
  if(model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.printf("ERROR: Model schema version %d not supported (expected %d)\n", 
                  model->version(), TFLITE_SCHEMA_VERSION);
    return false;
  }
  
  Serial.printf("Model loaded from flash: %d bytes\n", model_tflite_len);
  Serial.printf("Model version: %d\n", model->version());
  
  // Print model header for verification
  Serial.print("Model header: ");
  for(int i = 0; i < 16; i++) {
    Serial.printf("%02X ", model_tflite[i]);
  }
  Serial.println();
  
  // Create op resolver
  static tflite::AllOpsResolver resolver;
  Serial.println("Op resolver created");
  
  // Build interpreter
  Serial.println("Creating interpreter...");
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;
  Serial.println("Interpreter created");
  
  // Allocate tensors
  Serial.println("Allocating tensors...");
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if(allocate_status != kTfLiteOk) {
    Serial.println("ERROR: AllocateTensors() failed");
    return false;
  }
  Serial.println("Tensors allocated successfully");
  
  // Get input/output tensors
  input = interpreter->input(0);
  output = interpreter->output(0);
  
  if(!input || !output) {
    Serial.println("ERROR: Failed to get tensors");
    return false;
  }
  
  // Get quantization parameters
  input_scale = input->params.scale;
  input_zero_point = input->params.zero_point;
  output_scale = output->params.scale;
  output_zero_point = output->params.zero_point;
  
  Serial.println("\n=== Tensor Information ===");
  Serial.printf("Input tensor:\n");
  Serial.printf("  Type: %d\n", input->type);
  Serial.printf("  Bytes: %d\n", input->bytes);
  Serial.printf("  Scale: %.8f\n", input_scale);
  Serial.printf("  Zero point: %d\n", input_zero_point);
  
  Serial.printf("Output tensor:\n");
  Serial.printf("  Type: %d\n", output->type);
  Serial.printf("  Bytes: %d\n", output->bytes);
  Serial.printf("  Scale: %.8f\n", output_scale);
  Serial.printf("  Zero point: %d\n", output_zero_point);
  
  Serial.printf("\nArena used: %d / %d bytes (%.1f%%)\n", 
                interpreter->arena_used_bytes(), 
                kTensorArenaSize,
                100.0 * interpreter->arena_used_bytes() / kTensorArenaSize);
  
  Serial.println("\nSUCCESS: TFLite initialized");
  return true;
}

// Run inference (matches Python output format)
void runInference(float input_value) {
  // Quantize input
  int8_t quantized_input = quantize(input_value, input_scale, input_zero_point);
  
  // Clear input buffer and set value
  memset(input->data.int8, 0, input->bytes);
  input->data.int8[0] = quantized_input;
  
  // Run inference
  TfLiteStatus invoke_status = interpreter->Invoke();
  if(invoke_status != kTfLiteOk) {
    Serial.println("ERROR: Invoke failed");
    return;
  }
  
  // Get raw outputs
  int8_t raw_0 = output->data.int8[0];
  int8_t raw_1 = output->data.int8[1];
  
  // Dequantize to probabilities
  float prob_0 = dequantize(raw_0, output_scale, output_zero_point);
  float prob_1 = dequantize(raw_1, output_scale, output_zero_point);
  
  // Predict class
  int predicted = (prob_0 > prob_1) ? 0 : 1;
  
  // Output in same format as Python for easy comparison
  Serial.printf("Input: %5.1f (q:%4d) -> Raw:[%4d, %4d] -> Class 0: %.4f, Class 1: %.4f -> Predicted: %d\n",
                input_value, quantized_input, raw_0, raw_1, prob_0, prob_1, predicted);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n");
  Serial.println("============================================================");
  Serial.println("   ESP32 TensorFlow Lite - Embedded Model Serial Test");
  Serial.println("============================================================");
  
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  
  // Initialize TensorFlow Lite
  if(!initializeTFLite()) {
    Serial.println("\nFATAL: TFLite initialization failed!");
    return;
  }
  
  Serial.printf("Free heap after TFLite init: %d bytes\n", ESP.getFreeHeap());
  
  float test_values[] = {0.5, 0.8, 1.0, 1.2, 1.5, 1.8, 2.0, 2.2, 2.5};
  for(int i = 0; i < 9; i++) {
    runInference(test_values[i]);
    delay(100);
  }
  
  Serial.println("------------------------------------------------------------");
  Serial.println("\n============================================================");
  Serial.println("   Manual Test Mode");
  Serial.println("============================================================");
  Serial.println("Send float values for inference (e.g., 1.0, 2.0)");
  Serial.println("============================================================\n");
}

void loop() {
  // Read serial data
  while(Serial.available()) {
    char inChar = (char)Serial.read();
    
    if(inChar == '\n' || inChar == '\r') {
      if(inputString.length() > 0) {
        stringComplete = true;
      }
    } else {
      inputString += inChar;
    }
  }
  
  // Process complete command
  if(stringComplete) {
    inputString.trim();
    
    if(inputString.length() > 0) {
      float input_value = inputString.toFloat();
      
      if(input_value != 0.0 || inputString == "0" || inputString == "0.0") {
        Serial.println();
        runInference(input_value);
      } else {
        Serial.println("ERROR: Invalid input - send a number (e.g., 1.0, 2.0)");
      }
    }
    
    inputString = "";
    stringComplete = false;
  }
  
  delay(10);
}