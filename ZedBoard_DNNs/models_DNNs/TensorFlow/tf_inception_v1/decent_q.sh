decent_q quantize \
  --input_frozen_graph ./float/inception_v1_inference.pb \
  --input_nodes input \
  --input_shapes ?,224,224,3 \
  --output_nodes InceptionV1/Logits/Predictions/Reshape_1 \
  --input_fn inception_v1_input_fn.calib_input \
  --method 1 \
  --gpu 0 \
  --calib_iter 100 \