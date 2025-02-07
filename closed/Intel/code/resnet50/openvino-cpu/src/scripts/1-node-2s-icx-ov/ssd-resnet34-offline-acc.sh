#!/bin/bash

export OV_MLPERF_BIN=</path/to/ov_mlperf>

${OV_MLPERF_BIN} --scenario Offline \
        --mode Accuracy \
        --mlperf_conf mlperf.conf \
        --user_conf user.conf \
        --model_name ssd-resnet34 \
        --dataset coco \
        --data_path </path/to/dataset-coco-2017-val> \
        --model_path </path/to/ssd-resnet34_int8.xml> \
        --total_sample_count 5000 \
        --nireq 20 \
        --nstreams 16 \
        --nthreads 80 \
        --batch_size 1

