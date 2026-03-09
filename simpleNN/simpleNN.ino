// #include "sd_read_write.h"
// #include "SD_MMC.h"
// #include "TensorFlowLite_ESP32.h"
// #include "tensorflow/lite/micro/micro_error_reporter.h"
// #include "tensorflow/lite/micro/micro_interpreter.h"
// #include "tensorflow/lite/micro/all_ops_resolver.h"
// #include "tensorflow/lite/schema/schema_generated.h"

// // SD Card pins
// #define SD_MMC_CMD 15
// #define SD_MMC_CLK 14
// #define SD_MMC_D0  2

// // TensorFlow Lite globals
// namespace {
//   tflite::ErrorReporter* error_reporter = nullptr;
//   const tflite::Model* model = nullptr;
//   tflite::MicroInterpreter* interpreter = nullptr;
//   TfLiteTensor* input = nullptr;
//   TfLiteTensor* output = nullptr;
  
//   // Quantization parameters
//   float input_scale = 0.0f;
//   int32_t input_zero_point = 0;
//   float output_scale = 0.0f;
//   int32_t output_zero_point = 0;
  
//   // Memory arena
//   constexpr int kTensorArenaSize = 30 * 1024;
//   alignas(16) uint8_t tensor_arena[kTensorArenaSize];
  
//   // Buffer to hold model data
//   uint8_t* model_data = nullptr;
//   size_t model_size = 0;
// }

// // Serial input buffer
// String inputString = "";
// bool stringComplete = false;

// // Function to load model from SD card
// bool loadModelFromSD(const char* model_path) {
//   Serial.printf("Loading model from: %s\n", model_path);
  
//   File file = SD_MMC.open(model_path);
//   if(!file) {
//     Serial.println("ERROR: Failed to open model file");
//     return false;
//   }
  
//   model_size = file.size();
//   Serial.printf("Model size: %d bytes\n", model_size);
  
//   model_data = (uint8_t*)malloc(model_size);
//   if(!model_data) {
//     Serial.println("ERROR: Failed to allocate memory for model");
//     file.close();
//     return false;
//   }
  
//   size_t bytes_read = file.read(model_data, model_size);
//   file.close();
  
//   if(bytes_read != model_size) {
//     Serial.printf("ERROR: Read error - expected %d bytes, got %d\n", model_size, bytes_read);
//     free(model_data);
//     model_data = nullptr;
//     return false;
//   }
  
//   Serial.println("SUCCESS: Model loaded");
//   return true;
// }

// // Function to initialize TensorFlow Lite
// bool initializeTFLite() {
//   Serial.println("\n--- Initializing TensorFlow Lite ---");
  
//   // Set up error reporter
//   static tflite::MicroErrorReporter micro_error_reporter;
//   error_reporter = &micro_error_reporter;
//   Serial.println("Error reporter created");
  
//   // Map the model
//   model = tflite::GetModel(model_data);
//   if(model->version() != TFLITE_SCHEMA_VERSION) {
//     Serial.printf("ERROR: Model schema version %d not supported (expected %d)\n",
//                   model->version(), TFLITE_SCHEMA_VERSION);
//     return false;
//   }
//   Serial.println("Model schema version OK");
  
//   // Create op resolver
//   static tflite::AllOpsResolver resolver;
//   Serial.println("Op resolver created");
  
//   // Build interpreter
//   Serial.println("Creating interpreter...");
//   static tflite::MicroInterpreter static_interpreter(
//       model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
//   interpreter = &static_interpreter;
//   Serial.println("Interpreter created");
  
//   // Allocate tensors
//   Serial.println("Allocating tensors...");
//   TfLiteStatus allocate_status = interpreter->AllocateTensors();
//   if(allocate_status != kTfLiteOk) {
//     TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
//     Serial.printf("ERROR: AllocateTensors() failed with status: %d\n", allocate_status);
//     return false;
//   }
//   Serial.println("Tensors allocated successfully");
  
//   // Get input/output tensors
//   input = interpreter->input(0);
//   output = interpreter->output(0);
  
//   if(!input || !output) {
//     Serial.println("ERROR: Failed to get input/output tensors");
//     return false;
//   }
  
//   // Get quantization parameters
//   input_scale = input->params.scale;
//   input_zero_point = input->params.zero_point;
//   output_scale = output->params.scale;
//   output_zero_point = output->params.zero_point;
  
//   Serial.println("\n--- Tensor Information ---");
//   Serial.printf("Input tensor:\n");
//   Serial.printf("  Type: %d\n", input->type);
//   Serial.printf("  Bytes: %d\n", input->bytes);
//   if(input->dims) {
//     Serial.printf("  Dims: %d\n", input->dims->size);
//     for(int i = 0; i < input->dims->size; i++) {
//       Serial.printf("    [%d] = %d\n", i, input->dims->data[i]);
//     }
//   }
//   Serial.printf("  Scale: %.6f\n", input_scale);
//   Serial.printf("  Zero point: %d\n", input_zero_point);
  
//   Serial.printf("Output tensor:\n");
//   Serial.printf("  Type: %d\n", output->type);
//   Serial.printf("  Bytes: %d\n", output->bytes);
//   if(output->dims) {
//     Serial.printf("  Dims: %d\n", output->dims->size);
//     for(int i = 0; i < output->dims->size; i++) {
//       Serial.printf("    [%d] = %d\n", i, output->dims->data[i]);
//     }
//   }
//   Serial.printf("  Scale: %.6f\n", output_scale);
//   Serial.printf("  Zero point: %d\n", output_zero_point);
  
//   Serial.printf("\nArena used: %d bytes (of %d bytes)\n", 
//                 interpreter->arena_used_bytes(), kTensorArenaSize);
  
//   Serial.println("\nSUCCESS: TFLite initialized");
//   return true;
// }

// // Function to quantize float value to int8
// int8_t quantize(float value, float scale, int32_t zero_point) {
//   int32_t quantized = round(value / scale + zero_point);
//   if(quantized < -128) quantized = -128;
//   if(quantized > 127) quantized = 127;
//   return (int8_t)quantized;
// }

// // Function to dequantize int8 value to float
// float dequantize(int8_t value, float scale, int32_t zero_point) {
//   return (value - zero_point) * scale;
// }

// // Function to run inference
// void runInference(float input_value) {
//   // Quantize input
//   int8_t quantized_input = quantize(input_value, input_scale, input_zero_point);
  
//   // Set input tensor
//   input->data.int8[0] = quantized_input;
  
//   // Run inference
//   TfLiteStatus invoke_status = interpreter->Invoke();
//   if(invoke_status != kTfLiteOk) {
//     Serial.println("ERROR: Invoke failed");
//     TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed");
//     return;
//   }
  
//   // Get outputs
//   int8_t output_int8_0 = output->data.int8[0];
//   int8_t output_int8_1 = output->data.int8[1];
  
//   // Dequantize outputs
//   float score_0 = dequantize(output_int8_0, output_scale, output_zero_point);
//   float score_1 = dequantize(output_int8_1, output_scale, output_zero_point);
  
//   // Determine predicted class
//   int predicted_class = (score_0 > score_1) ? 0 : 1;
  
//   // Send results
//   Serial.printf("RESULT|input:%.4f|quantized:%d|class0:%.6f|class1:%.6f|predicted:%d\n",
//                 input_value, quantized_input, score_0, score_1, predicted_class);
// }

// void setup() {
//   Serial.begin(115200);
//   delay(1000);
//   Serial.println();
//   Serial.println("==============================");
//   Serial.println("ESP32 TFLite Serial Test Mode");
//   Serial.println("==============================");
  
//   // Print memory info
//   Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  
//   // Initialize SD card
//   Serial.println("\nInitializing SD card...");
//   SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
//   if(!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)) {
//     Serial.println("ERROR: Card Mount Failed");
//     return;
//   }
//   Serial.println("SUCCESS: SD card initialized");
  
//   // Load model
//   if(!loadModelFromSD("/model_int8.tflite")) {
//     Serial.println("FATAL: Failed to load model - stopping");
//     return;
//   }
  
//   Serial.printf("Free heap after model load: %d bytes\n", ESP.getFreeHeap());
  
//   // Initialize TensorFlow Lite
//   if(!initializeTFLite()) {
//     Serial.println("FATAL: Failed to initialize TFLite - stopping");
//     return;
//   }
  
//   Serial.printf("Free heap after TFLite init: %d bytes\n", ESP.getFreeHeap());
  
//   Serial.println();
//   Serial.println("==============================");
//   Serial.println("READY: Send float values for inference");
//   Serial.println("Example: 1.0 or 2.0 or any float value");
//   Serial.println("==============================");
//   Serial.println();
// }

// void loop() {
//   // Read serial data
//   while(Serial.available()) {
//     char inChar = (char)Serial.read();
    
//     if(inChar == '\n' || inChar == '\r') {
//       if(inputString.length() > 0) {
//         stringComplete = true;
//       }
//     } else {
//       inputString += inChar;
//     }
//   }
  
//   // Process complete command
//   if(stringComplete) {
//     inputString.trim();
    
//     if(inputString.length() > 0) {
//       float input_value = inputString.toFloat();
      
//       if(input_value != 0.0 || inputString == "0" || inputString == "0.0") {
//         Serial.printf("\n>>> Processing input: %.4f\n", input_value);
//         runInference(input_value);
//       } else {
//         Serial.println("ERROR: Invalid input - send a number (e.g., 1.0, 2.0)");
//       }
//     }
    
//     inputString = "";
//     stringComplete = false;
//   }
  
//   delay(10);
// }

// #include "sd_read_write.h"
// #include "SD_MMC.h"
// #include "TensorFlowLite_ESP32.h"
// #include "tensorflow/lite/micro/micro_error_reporter.h"
// #include "tensorflow/lite/micro/micro_interpreter.h"
// #include "tensorflow/lite/micro/all_ops_resolver.h"
// #include "tensorflow/lite/schema/schema_generated.h"

// // SD Card pins
// #define SD_MMC_CMD 15
// #define SD_MMC_CLK 14
// #define SD_MMC_D0  2

// // TensorFlow Lite globals
// namespace {
//   tflite::ErrorReporter* error_reporter = nullptr;
//   const tflite::Model* model = nullptr;
//   tflite::MicroInterpreter* interpreter = nullptr;
//   TfLiteTensor* input = nullptr;
//   TfLiteTensor* output = nullptr;
  
//   // Quantization parameters
//   float input_scale = 0.0f;
//   int32_t input_zero_point = 0;
//   float output_scale = 0.0f;
//   int32_t output_zero_point = 0;
  
//   // Memory arena
//   constexpr int kTensorArenaSize = 30 * 1024;
//   alignas(16) uint8_t tensor_arena[kTensorArenaSize];
  
//   // Buffer to hold model data
//   uint8_t* model_data = nullptr;
//   size_t model_size = 0;
// }

// // Serial input buffer
// String inputString = "";
// bool stringComplete = false;

// // Function to load model from SD card
// bool loadModelFromSD(const char* model_path) {
//   Serial.printf("Loading model from: %s\n", model_path);
  
//   File file = SD_MMC.open(model_path);
//   if(!file) {
//     Serial.println("ERROR: Failed to open model file");
//     return false;
//   }
  
//   model_size = file.size();
//   Serial.printf("Model size: %d bytes\n", model_size);
  
//   model_data = (uint8_t*)malloc(model_size);
//   if(!model_data) {
//     Serial.println("ERROR: Failed to allocate memory for model");
//     file.close();
//     return false;
//   }
  
//   size_t bytes_read = file.read(model_data, model_size);
//   file.close();
  
//   if(bytes_read != model_size) {
//     Serial.printf("ERROR: Read error - expected %d bytes, got %d\n", model_size, bytes_read);
//     free(model_data);
//     model_data = nullptr;
//     return false;
//   }
  
//   // Verify model header
//   Serial.print("Model header (first 16 bytes): ");
//   for(int i = 0; i < 16 && i < model_size; i++) {
//     Serial.printf("%02X ", model_data[i]);
//   }
//   Serial.println();
  
//   Serial.println("SUCCESS: Model loaded");
//   return true;
// }

// // Function to initialize TensorFlow Lite
// bool initializeTFLite() {
//   Serial.println("\n--- Initializing TensorFlow Lite ---");
  
//   // Set up error reporter
//   static tflite::MicroErrorReporter micro_error_reporter;
//   error_reporter = &micro_error_reporter;
  
//   // Map the model
//   model = tflite::GetModel(model_data);
//   if(model->version() != TFLITE_SCHEMA_VERSION) {
//     Serial.printf("ERROR: Model schema version %d not supported (expected %d)\n",
//                   model->version(), TFLITE_SCHEMA_VERSION);
//     return false;
//   }
//   Serial.printf("Model schema version: %d\n", model->version());
  
//   // Create op resolver
//   static tflite::AllOpsResolver resolver;
  
//   // Build interpreter
//   static tflite::MicroInterpreter static_interpreter(
//       model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
//   interpreter = &static_interpreter;
  
//   // Allocate tensors
//   TfLiteStatus allocate_status = interpreter->AllocateTensors();
//   if(allocate_status != kTfLiteOk) {
//     Serial.printf("ERROR: AllocateTensors() failed\n");
//     return false;
//   }
  
//   // Get input/output tensors
//   input = interpreter->input(0);
//   output = interpreter->output(0);
  
//   if(!input || !output) {
//     Serial.println("ERROR: Failed to get input/output tensors");
//     return false;
//   }
  
//   // Get quantization parameters
//   input_scale = input->params.scale;
//   input_zero_point = input->params.zero_point;
//   output_scale = output->params.scale;
//   output_zero_point = output->params.zero_point;
  
//   Serial.println("\n=== Tensor Info ===");
//   Serial.printf("Input: scale=%.8f, zero_point=%d\n", input_scale, input_zero_point);
//   Serial.printf("Output: scale=%.8f, zero_point=%d\n", output_scale, output_zero_point);
//   Serial.printf("Arena used: %d bytes\n", interpreter->arena_used_bytes());
  
//   Serial.println("\nSUCCESS: TFLite initialized");
//   return true;
// }

// // Quantize float to int8
// int8_t quantize(float value, float scale, int32_t zero_point) {
//   int32_t quantized = round(value / scale + zero_point);
//   if(quantized < -128) quantized = -128;
//   if(quantized > 127) quantized = 127;
//   return (int8_t)quantized;
// }

// // Dequantize int8 to float
// float dequantize(int8_t value, float scale, int32_t zero_point) {
//   return (float)(value - zero_point) * scale;
// }

// // Run inference with Python-matching output
// void runInference(float input_value) {
//   // Quantize input
//   int8_t quantized_input = quantize(input_value, input_scale, input_zero_point);
  
//   // Clear input buffer and set value
//   memset(input->data.int8, 0, input->bytes);
//   input->data.int8[0] = quantized_input;
  
//   // Run inference
//   TfLiteStatus invoke_status = interpreter->Invoke();
//   if(invoke_status != kTfLiteOk) {
//     Serial.println("ERROR: Invoke failed");
//     return;
//   }
  
//   // Get raw outputs
//   int8_t raw_0 = output->data.int8[0];
//   int8_t raw_1 = output->data.int8[1];
  
//   // Dequantize
//   float prob_0 = dequantize(raw_0, output_scale, output_zero_point);
//   float prob_1 = dequantize(raw_1, output_scale, output_zero_point);
  
//   // Predict
//   int predicted = (prob_0 > prob_1) ? 0 : 1;
  
//   // Format to match Python output
//   Serial.printf("Input: %5.3f (q:%4d) -> Raw:[%4d, %4d] -> Class 0: %.4f, Class 1: %.4f -> Predicted: %d\n",
//                 input_value, quantized_input, raw_0, raw_1, prob_0, prob_1, predicted);
// }

// void setup() {
//   Serial.begin(115200);
//   delay(1000);
//   Serial.println();
//   Serial.println("==============================");
//   Serial.println("ESP32 TFLite Serial Test Mode");
//   Serial.println("==============================");
  
//   Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  
//   // Initialize SD card
//   Serial.println("\nInitializing SD card...");
//   SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
//   if(!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)) {
//     Serial.println("ERROR: Card Mount Failed");
//     return;
//   }
//   Serial.println("SUCCESS: SD card initialized");
  
//   // Load model
//   if(!loadModelFromSD("/model_int8.tflite")) {
//     Serial.println("FATAL: Failed to load model");
//     return;
//   }
  
//   // Initialize TensorFlow Lite
//   if(!initializeTFLite()) {
//     Serial.println("FATAL: Failed to initialize TFLite");
//     return;
//   }
  
//   // Auto-run test suite on startup
//   Serial.println("\n=== Running Auto-Test ===");
//   float test_vals[] = {1.0, 1.5, 2.0};
//   for(int i = 0; i < 3; i++) {
//     runInference(test_vals[i]);
//     delay(100);
//   }
//   Serial.println("\nSend values for more tests (e.g., 1.0, 2.0)");
// }

// void loop() {
//   while(Serial.available()) {
//     char inChar = (char)Serial.read();
    
//     if(inChar == '\n' || inChar == '\r') {
//       if(inputString.length() > 0) {
//         stringComplete = true;
//       }
//     } else {
//       inputString += inChar;
//     }
//   }
  
//   if(stringComplete) {
//     inputString.trim();
    
//     if(inputString.length() > 0) {
//       float input_value = inputString.toFloat();
      
//       if(input_value != 0.0 || inputString == "0" || inputString == "0.0") {
//         runInference(input_value);
//       } else {
//         Serial.println("ERROR: Invalid input");
//       }
//     }
    
//     inputString = "";
//     stringComplete = false;
//   }
  
//   delay(10);
// }

#include "sd_read_write.h"
#include "SD_MMC.h"
#include "TensorFlowLite_ESP32.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

// SD Card pins
#define SD_MMC_CMD 15
#define SD_MMC_CLK 14
#define SD_MMC_D0  2

// Maximum supported dimensions
#define MAX_INPUT_SIZE 1000
#define MAX_OUTPUT_SIZE 100

// TensorFlow Lite globals
namespace {
  tflite::ErrorReporter* error_reporter = nullptr;
  const tflite::Model* model = nullptr;
  tflite::MicroInterpreter* interpreter = nullptr;
  TfLiteTensor* input = nullptr;
  TfLiteTensor* output = nullptr;
  
  // Tensor metadata
  int input_size = 0;
  int output_size = 0;
  TfLiteType input_type = kTfLiteNoType;
  TfLiteType output_type = kTfLiteNoType;
  
  // Quantization parameters
  float input_scale = 0.0f;
  int32_t input_zero_point = 0;
  float output_scale = 0.0f;
  int32_t output_zero_point = 0;
  
  // Memory arena - adjust size based on model complexity
  constexpr int kTensorArenaSize = 50 * 1024;  // 50KB
  alignas(16) uint8_t tensor_arena[kTensorArenaSize];
  
  // Buffer to hold model data
  uint8_t* model_data = nullptr;
  size_t model_size = 0;
}

// Serial input buffer
String inputString = "";
bool stringComplete = false;

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

// Get human-readable type name
const char* getTypeName(TfLiteType type) {
  switch(type) {
    case kTfLiteFloat32: return "FLOAT32";
    case kTfLiteInt8: return "INT8";
    case kTfLiteUInt8: return "UINT8";
    case kTfLiteInt16: return "INT16";
    case kTfLiteInt32: return "INT32";
    case kTfLiteInt64: return "INT64";
    case kTfLiteBool: return "BOOL";
    default: return "UNKNOWN";
  }
}

// Calculate total elements in a tensor
int getTensorSize(TfLiteTensor* tensor) {
  if(!tensor || !tensor->dims) return 0;
  
  int size = 1;
  for(int i = 0; i < tensor->dims->size; i++) {
    size *= tensor->dims->data[i];
  }
  return size;
}

// Print tensor shape
void printTensorShape(TfLiteTensor* tensor) {
  if(!tensor || !tensor->dims) {
    Serial.println("  Shape: Unknown");
    return;
  }
  
  Serial.print("  Shape: [");
  for(int i = 0; i < tensor->dims->size; i++) {
    Serial.print(tensor->dims->data[i]);
    if(i < tensor->dims->size - 1) Serial.print(", ");
  }
  Serial.println("]");
}

// =============================================================================
// MODEL LOADING
// =============================================================================

bool loadModelFromSD(const char* model_path) {
  Serial.printf("\n=== Loading Model ===\n");
  Serial.printf("Path: %s\n", model_path);
  
  if(!SD_MMC.exists(model_path)) {
    Serial.println("ERROR: Model file not found on SD card");
    return false;
  }
  
  File file = SD_MMC.open(model_path);
  if(!file) {
    Serial.println("ERROR: Failed to open model file");
    return false;
  }
  
  model_size = file.size();
  Serial.printf("Model size: %d bytes\n", model_size);
  
  if(model_size < 100 || model_size > 5000000) {
    Serial.printf("ERROR: Invalid model size: %d bytes\n", model_size);
    file.close();
    return false;
  }
  
  model_data = (uint8_t*)malloc(model_size);
  if(!model_data) {
    Serial.println("ERROR: Failed to allocate memory");
    file.close();
    return false;
  }
  
  size_t bytes_read = file.read(model_data, model_size);
  file.close();
  
  if(bytes_read != model_size) {
    Serial.printf("ERROR: Read error - expected %d, got %d bytes\n", 
                  model_size, bytes_read);
    free(model_data);
    model_data = nullptr;
    return false;
  }
  
  // Verify TFLite header (should start with "TFL3")
  if(model_size >= 4) {
    Serial.printf("Model header: %c%c%c%c\n", 
                  model_data[0], model_data[1], model_data[2], model_data[3]);
  }
  
  Serial.println("SUCCESS: Model loaded from SD card");
  return true;
}

// =============================================================================
// TENSORFLOW LITE INITIALIZATION
// =============================================================================

bool initializeTFLite() {
  Serial.println("\n=== Initializing TensorFlow Lite ===");
  
  // Set up error reporter
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;
  
  // Map the model
  model = tflite::GetModel(model_data);
  if(model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.printf("ERROR: Schema mismatch - model v%d, expected v%d\n",
                  model->version(), TFLITE_SCHEMA_VERSION);
    return false;
  }
  
  // Create resolver
  static tflite::AllOpsResolver resolver;
  
  // Build interpreter
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;
  
  // Allocate tensors
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if(allocate_status != kTfLiteOk) {
    Serial.println("ERROR: AllocateTensors() failed");
    return false;
  }
  
  // Get input/output tensors
  input = interpreter->input(0);
  output = interpreter->output(0);
  
  if(!input || !output) {
    Serial.println("ERROR: Failed to get tensors");
    return false;
  }
  
  // Store metadata
  input_type = input->type;
  output_type = output->type;
  input_size = getTensorSize(input);
  output_size = getTensorSize(output);
  
  // Get quantization parameters (if quantized)
  if(input_type == kTfLiteInt8 || input_type == kTfLiteUInt8) {
    input_scale = input->params.scale;
    input_zero_point = input->params.zero_point;
  }
  
  if(output_type == kTfLiteInt8 || output_type == kTfLiteUInt8) {
    output_scale = output->params.scale;
    output_zero_point = output->params.zero_point;
  }
  
  // Print detailed information
  Serial.println("\n=== INPUT TENSOR ===");
  Serial.printf("  Type: %s\n", getTypeName(input_type));
  Serial.printf("  Elements: %d\n", input_size);
  printTensorShape(input);
  if(input_type == kTfLiteInt8 || input_type == kTfLiteUInt8) {
    Serial.printf("  Scale: %.8f\n", input_scale);
    Serial.printf("  Zero Point: %d\n", input_zero_point);
  }
  
  Serial.println("\n=== OUTPUT TENSOR ===");
  Serial.printf("  Type: %s\n", getTypeName(output_type));
  Serial.printf("  Elements: %d\n", output_size);
  printTensorShape(output);
  if(output_type == kTfLiteInt8 || output_type == kTfLiteUInt8) {
    Serial.printf("  Scale: %.8f\n", output_scale);
    Serial.printf("  Zero Point: %d\n", output_zero_point);
  }
  
  Serial.printf("\nArena used: %d / %d bytes (%.1f%%)\n", 
                interpreter->arena_used_bytes(), 
                kTensorArenaSize,
                100.0 * interpreter->arena_used_bytes() / kTensorArenaSize);
  
  // Validate sizes
  if(input_size > MAX_INPUT_SIZE) {
    Serial.printf("ERROR: Input size %d exceeds maximum %d\n", 
                  input_size, MAX_INPUT_SIZE);
    return false;
  }
  
  if(output_size > MAX_OUTPUT_SIZE) {
    Serial.printf("ERROR: Output size %d exceeds maximum %d\n", 
                  output_size, MAX_OUTPUT_SIZE);
    return false;
  }
  
  Serial.println("\nSUCCESS: TFLite initialized and ready");
  return true;
}

// =============================================================================
// QUANTIZATION FUNCTIONS
// =============================================================================

// Generic quantization to int8
int8_t quantizeToInt8(float value, float scale, int32_t zero_point) {
  int32_t quantized = round(value / scale + zero_point);
  Serial.printf("");
  return (int8_t)constrain(quantized, -128, 127);
}

// Generic quantization to uint8
uint8_t quantizeToUInt8(float value, float scale, int32_t zero_point) {
  int32_t quantized = round(value / scale + zero_point);
  return (uint8_t)constrain(quantized, 0, 255);
}

// Generic dequantization from int8
float dequantizeFromInt8(int8_t value, float scale, int32_t zero_point) {
  return (float)(value - zero_point) * scale;
}

// Generic dequantization from uint8
float dequantizeFromUInt8(uint8_t value, float scale, int32_t zero_point) {
  return (float)(value - zero_point) * scale;
}

// =============================================================================
// INPUT HANDLING
// =============================================================================

// Set input tensor from float array
bool setInputFromFloatArray(float* values, int count) {
  if(count > input_size) {
    Serial.printf("ERROR: Too many inputs - got %d, expected %d\n", 
                  count, input_size);
    return false;
  }
  
  // Clear input buffer
  memset(input->data.raw, 0, input->bytes);
  
  // Set values based on input type
  switch(input_type) {
    case kTfLiteFloat32:
      for(int i = 0; i < count; i++) {
        input->data.f[i] = values[i];
      }
      break;
      
    case kTfLiteInt8:
      for(int i = 0; i < count; i++) {
        input->data.int8[i] = quantizeToInt8(values[i], input_scale, 
                                             input_zero_point);
      }
      break;
      
    case kTfLiteUInt8:
      for(int i = 0; i < count; i++) {
        input->data.uint8[i] = quantizeToUInt8(values[i], input_scale, 
                                               input_zero_point);
      }
      break;
      
    case kTfLiteInt32:
      for(int i = 0; i < count; i++) {
        input->data.i32[i] = (int32_t)round(values[i]);
      }
      break;
      
    default:
      Serial.printf("ERROR: Unsupported input type: %s\n", 
                    getTypeName(input_type));
      return false;
  }
  
  return true;
}

// =============================================================================
// OUTPUT HANDLING
// =============================================================================

// Get output as float array
bool getOutputAsFloatArray(float* values, int max_count) {
  int count = min(output_size, max_count);
  
  switch(output_type) {
    case kTfLiteFloat32:
      for(int i = 0; i < count; i++) {
        values[i] = output->data.f[i];
      }
      break;
      
    case kTfLiteInt8:
      for(int i = 0; i < count; i++) {
        values[i] = dequantizeFromInt8(output->data.int8[i], 
                                       output_scale, output_zero_point);
      }
      break;
      
    case kTfLiteUInt8:
      for(int i = 0; i < count; i++) {
        values[i] = dequantizeFromUInt8(output->data.uint8[i], 
                                        output_scale, output_zero_point);
      }
      break;
      
    case kTfLiteInt32:
      for(int i = 0; i < count; i++) {
        values[i] = (float)output->data.i32[i];
      }
      break;
      
    default:
      Serial.printf("ERROR: Unsupported output type: %s\n", 
                    getTypeName(output_type));
      return false;
  }
  
  return true;
}

// =============================================================================
// INFERENCE
// =============================================================================

bool runInference(float* input_values, int input_count, 
                  float* output_values, int max_output_count) {
  // Set input
  if(!setInputFromFloatArray(input_values, input_count)) {
    return false;
  }
  
  // Run inference
  TfLiteStatus invoke_status = interpreter->Invoke();
  if(invoke_status != kTfLiteOk) {
    Serial.println("ERROR: Invoke() failed");
    return false;
  }
  
  // Get output
  if(!getOutputAsFloatArray(output_values, max_output_count)) {
    return false;
  }
  
  return true;
}

// =============================================================================
// CLASSIFICATION HELPER
// =============================================================================

// For classification models - returns predicted class and confidence
int classifyWithConfidence(float* input_values, int input_count, 
                          float* confidence) {
  // Allocate on heap instead of stack
  float* outputs = (float*)malloc(output_size * sizeof(float));
  if(!outputs) {
    Serial.println("ERROR: Failed to allocate output buffer");
    *confidence = 0.0f;
    return -1;
  }
  
  if(!runInference(input_values, input_count, outputs, output_size)) {
    *confidence = 0.0f;
    free(outputs);
    return -1;
  }
  
  // Find max output (predicted class)
  int predicted_class = 0;
  float max_value = outputs[0];
  
  for(int i = 1; i < output_size; i++) {
    if(outputs[i] > max_value) {
      max_value = outputs[i];
      predicted_class = i;
    }
  }
  
  *confidence = max_value;
  free(outputs);
  return predicted_class;
}

// =============================================================================
// DEMO/TEST FUNCTIONS
// =============================================================================

void runSingleValueTest(float input_value) {
  Serial.printf("\n>>> Testing input: %.4f\n", input_value);
  
  float input_arr[1] = {input_value};
  float* output_arr = (float*)malloc(output_size * sizeof(float));
  
  if(!output_arr) {
    Serial.println("ERROR: Failed to allocate output buffer");
    return;
  }
  
  if(!runInference(input_arr, 1, output_arr, output_size)) {
    Serial.println("ERROR: Inference failed");
    free(output_arr);
    return;
  }
  
  Serial.print("Outputs: [");
  for(int i = 0; i < output_size; i++) {
    Serial.printf("%.4f", output_arr[i]);
    if(i < output_size - 1) Serial.print(", ");
  }
  Serial.println("]");
  
  // If it's a classification model (multiple outputs)
  if(output_size > 1) {
    float confidence;
    int predicted = classifyWithConfidence(input_arr, 1, &confidence);
    Serial.printf("Predicted Class: %d (confidence: %.2f%%)\n", 
                  predicted, confidence * 100);
  }
  
  free(output_arr);
}

// =============================================================================
// SETUP & LOOP
// =============================================================================

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.printf("\nFree heap: %d bytes\n", ESP.getFreeHeap());
  
  // Initialize SD card
  Serial.println("\n=== Initializing SD Card ===");
  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
  
  if(!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)) {
    Serial.println("ERROR: SD Card mount failed");
    return;
  }
  
  Serial.println("SUCCESS: SD card initialized");
  
  // Load model
  // if(!loadModelFromSD("/model_int8.tflite")) {
  //   Serial.println("FATAL: Failed to load model");
  //   return;
  // }
  if(!loadModelFromSD("/model_float32.tflite")) {
    Serial.println("FATAL: Failed to load model");
    return;
  }
  
  Serial.printf("Free heap after model load: %d bytes\n", ESP.getFreeHeap());
  
  // Initialize TensorFlow Lite
  if(!initializeTFLite()) {
    Serial.println("FATAL: Failed to initialize TFLite");
    return;
  }
  
  Serial.printf("Free heap after TFLite init: %d bytes\n", ESP.getFreeHeap());
  
  // Run auto-tests
  Serial.println("\n=== Running Auto-Test ===");
  float test_vals[] = {0.5, 1.0, 1.5, 2.0, 2.5};
  for(int i = 0; i < 5; i++) {
    runSingleValueTest(test_vals[i]);
    delay(100);
  }
  Serial.println("============================================================");
  Serial.println("Commands:");
  Serial.println("  Single value: 1.0");
  Serial.println("  Multiple values: 1.0,2.0,3.0");
  Serial.println("  Info: info");
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
  
  // Process command
  if(stringComplete) {
    inputString.trim();
    
    if(inputString.length() > 0) {
      if(inputString.equalsIgnoreCase("info")) {
        // Print model info
        Serial.println("\n=== MODEL INFO ===");
        Serial.printf("Input: %s, size=%d\n", getTypeName(input_type), input_size);
        Serial.printf("Output: %s, size=%d\n", getTypeName(output_type), output_size);
        Serial.printf("Memory: %d/%d bytes used\n", 
                      interpreter->arena_used_bytes(), kTensorArenaSize);
      } else {
        // Parse input values (comma-separated)
        // Use heap allocation to avoid stack overflow
        float* input_vals = (float*)malloc(input_size * sizeof(float));
        if(!input_vals) {
          Serial.println("ERROR: Failed to allocate input buffer");
          inputString = "";
          stringComplete = false;
        }
        
        int count = 0;
        
        int start = 0;
        while(start < inputString.length() && count < input_size) {
          int comma = inputString.indexOf(',', start);
          if(comma == -1) comma = inputString.length();
          
          String token = inputString.substring(start, comma);
          token.trim();
          input_vals[count++] = token.toFloat();
          
          start = comma + 1;
        }
        
        if(count == 1) {
          runSingleValueTest(input_vals[0]);
        } else if(count > 1) {
          Serial.printf("\n>>> Testing %d inputs\n", count);
          float* outputs = (float*)malloc(output_size * sizeof(float));
          if(outputs) {
            if(runInference(input_vals, count, outputs, output_size)) {
              Serial.print("Outputs: [");
              for(int i = 0; i < output_size; i++) {
                Serial.printf("%.4f", outputs[i]);
                if(i < output_size - 1) Serial.print(", ");
              }
              Serial.println("]");
            }
            free(outputs);
          } else {
            Serial.println("ERROR: Failed to allocate output buffer");
          }
        }
        
        free(input_vals);
      }
    }
    
    inputString = "";
    stringComplete = false;
  }
  
  delay(10);
}