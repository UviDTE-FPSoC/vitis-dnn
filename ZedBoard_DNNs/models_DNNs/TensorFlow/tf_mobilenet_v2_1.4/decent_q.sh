decent_q quantize \
  --input_frozen_graph ./float/mobilenet_v2_1.4_224.pb \
  --input_nodes input \
  --input_shapes ?,224,224,3 \
  --output_nodes MobilenetV2/Predictions/Reshape_1 \
  --input_fn mobilenet_v2_input_fn.calib_input \
  --method 1 \
  --gpu 0 \
  --calib_iter 100 \
