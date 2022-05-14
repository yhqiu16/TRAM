# SPDLOG_LEVEL=trace, debug, info, warn, err, critical, off
./build/cgra-compiler SPDLOG_LEVEL=off \
	-c true -m true -o true -t 3600000 -i 2000 \
	-p "../cgra-mg/src/main/resources/operations.json" \
	-a "../cgra-mg/src/main/resources/cgra_adg.json" \
	-d "../bechmarks/test/arf/arf.json ../bechmarks/test/centro-fir/centro-fir.json ../bechmarks/test/cosine1/cosine1.json ../bechmarks/test/ewf/ewf.json ../bechmarks/test/fft/fft.json ../bechmarks/test/fir1/fir1.json ../bechmarks/test/resnet1/resnet1.json"
	# -d "../bechmarks/test/cosine2/cosine2.json ../bechmarks/test/fir/fir.json ../bechmarks/test/md/md.json ../bechmarks/test/resnet2/resnet2.json ../bechmarks/test/stencil3d/stencil3d.json"	
	# -d "../bechmarks/test/cosine2/cosine2.json ../bechmarks/test/resnet2/resnet2.json ../bechmarks/test/stencil3d/stencil3d.json"
	# -d "../bechmarks/test/arf/arf.json ../bechmarks/test/centro-fir/centro-fir.json ../bechmarks/test/cosine1/cosine1.json ../bechmarks/test/ewf/ewf.json ../bechmarks/test/fft/fft.json ../bechmarks/test/fir/fir.json ../bechmarks/test/fir1/fir1.json ../bechmarks/test/resnet1/resnet1.json"
	# -d "../bechmarks/test/ewf/ewf.json"
	# -d "../bechmarks/test2/conv3/conv3.json ../bechmarks/test2/matrixmul/matrixmul.json ../bechmarks/test2/simple/simple.json"