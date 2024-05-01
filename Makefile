LLVMCODE = part3
TEST = test

$(LLVMCODE): $(LLVMCODE).c
	g++ -g -I /usr/include/llvm-c-15/ -c $(LLVMCODE).c
	g++ -g $(LLVMCODE).o `llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15/ -o $@

test: $(LLVMCODE)
	./$(LLVMCODE) /thayerfs/home/f0055xj/cs57-dev/part3/optimizer_test_results/p3_const_prop.ll

valgrind: $(LLVMCODE)
	valgrind ./$(LLVMCODE) /thayerfs/home/f0055xj/cs57-dev/part3/optimizer_test_results/p4_const_prop.ll

clean: 
	rm -rf $(TEST).ll
	rm -rf $(LLVMCODE)
	rm -rf *.o