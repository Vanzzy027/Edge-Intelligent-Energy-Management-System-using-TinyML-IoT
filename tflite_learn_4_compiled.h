
// Generated on: 31.03.2025 06:31:59

#ifndef tflite_learn_4_GEN_H
#define tflite_learn_4_GEN_H

#include "edge-impulse-sdk/tensorflow/lite/c/common.h"

// Sets up the model with init and prepare steps.
TfLiteStatus tflite_learn_4_init( void*(*alloc_fnc)(size_t,size_t) );
// Returns the input tensor with the given index.
TfLiteStatus tflite_learn_4_input(int index, TfLiteTensor* tensor);
// Returns the output tensor with the given index.
TfLiteStatus tflite_learn_4_output(int index, TfLiteTensor* tensor);
// Runs inference for the model.
TfLiteStatus tflite_learn_4_invoke();
//Frees memory allocated
TfLiteStatus tflite_learn_4_reset( void (*free)(void* ptr) );


// Returns the number of input tensors.
inline size_t tflite_learn_4_inputs() {
  return 1;
}
// Returns the number of output tensors.
inline size_t tflite_learn_4_outputs() {
  return 1;
}

#endif
