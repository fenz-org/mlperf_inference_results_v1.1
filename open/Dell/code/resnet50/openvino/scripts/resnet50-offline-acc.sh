echo "Test start time: $(date)"
./ov_mlperf --scenario Offline \
	--mode Accuracy \
	--mlperf_conf Configs/mlperf.conf \
	--user_conf Configs/resnet50/user.conf \
	--model_name resnet50 \
  --device CPU \
	--data_path /home/dell/CK-TOOLS/dataset-imagenet-ilsvrc2012-val \
  --dataset imagenet \
	--nireq 224 \
	--nthreads 56 \
	--nstreams 56 \
	--total_sample_count 1024 \
	--warmup_iters 50 \
	--model_path Models/resnet50/resnet50_int8.xml
echo "Test end time: $(date)"
