#include <cstdlib>
#include <iostream>
#include <string>

#include "llvm-c/Analysis.h"
#include "llvm-c/Core.h"
#include "llvm-c/Target.h"
#include "llvm-c/TargetMachine.h"

struct ExampleState {
	LLVMContextRef context = nullptr;
	LLVMModuleRef module = nullptr;
	LLVMBuilderRef builder = nullptr;
	LLVMTargetMachineRef target_machine = nullptr;
	char* target_triple = nullptr;
	std::string ir_path = "aot_example_gen.ll";
	#ifdef _WIN32
	std::string object_path = "aot_example_gen.obj";
	std::string executable_path = "aot_example_gen.exe";
	#else
	std::string object_path = "aot_example_gen.o";
	std::string executable_path = "aot_example_gen.out";
	#endif
};

std::string shell_quote(const std::string& value) {
	#ifdef _WIN32
	std::string quoted = "\"";
	for (char ch : value) {
		quoted += (ch == '"') ? "\"\"" : std::string(1, ch);
	}
	quoted += '"';
	#else
	std::string quoted = "'";
	for (char ch : value) {
		if (ch == '\'') {
			quoted += "'\\''";
		} else {
			quoted += ch;
		}
	}
	quoted += "'";
	#endif
	return quoted;
}

void initialize_llvm() {
	LLVMInitializeNativeTarget();
	LLVMInitializeNativeAsmPrinter();
	LLVMInitializeNativeAsmParser();
}

void configure_output_paths(ExampleState& state, int argc, char** argv) {
	if (argc < 2) {
		return;
	}

	std::string stem = argv[1];
	state.ir_path = stem + ".ll";
	#ifdef _WIN32
	state.object_path = stem + ".obj";
	state.executable_path = stem + ".exe";
	#else
	state.object_path = stem + ".o";
	state.executable_path = stem;
	#endif
}

void create_module_state(ExampleState& state) {
	state.context = LLVMContextCreate();
	state.module = LLVMModuleCreateWithNameInContext("aot_example_c", state.context);
	state.builder = LLVMCreateBuilderInContext(state.context);
}

bool configure_target_machine(ExampleState& state) {
	state.target_triple = LLVMGetDefaultTargetTriple();

	LLVMTargetRef target = nullptr;
	char* error_message = nullptr;
	if (LLVMGetTargetFromTriple(state.target_triple, &target, &error_message) != 0) {
		std::cerr << "Failed to resolve native target: " << error_message << std::endl;
		LLVMDisposeMessage(error_message);
		return false;
	}

	state.target_machine = LLVMCreateTargetMachine(
			target,
			state.target_triple,
			"generic",
			"",
			LLVMCodeGenLevelDefault,
			LLVMRelocPIC,
			LLVMCodeModelDefault);
	if (state.target_machine == nullptr) {
		std::cerr << "Failed to create target machine" << std::endl;
		return false;
	}

	LLVMSetTarget(state.module, state.target_triple);
	LLVMTargetDataRef data_layout = LLVMCreateTargetDataLayout(state.target_machine);
	LLVMSetModuleDataLayout(state.module, data_layout);
	LLVMDisposeTargetData(data_layout);
	return true;
}

void build_ir(ExampleState& state) {
	LLVMTypeRef i32_type = LLVMInt32TypeInContext(state.context);
	LLVMTypeRef i8_ptr_type = LLVMPointerType(LLVMInt8TypeInContext(state.context), 0);

	LLVMTypeRef puts_params[] = {i8_ptr_type};
	LLVMTypeRef puts_type = LLVMFunctionType(i32_type, puts_params, 1, 0);
	LLVMValueRef puts_fn = LLVMAddFunction(state.module, "puts", puts_type);

	LLVMTypeRef main_type = LLVMFunctionType(i32_type, nullptr, 0, 0);
	LLVMValueRef main_fn = LLVMAddFunction(state.module, "main", main_type);
	LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(state.context, main_fn, "entry");
	LLVMPositionBuilderAtEnd(state.builder, entry);

	LLVMValueRef hello = LLVMBuildGlobalStringPtr(state.builder, "hello world", "hello_str");
	LLVMBuildCall2(state.builder, puts_type, puts_fn, &hello, 1, "puts_result");
	LLVMBuildRet(state.builder, LLVMConstInt(i32_type, 0, 0));
}

bool verify_ir(ExampleState& state) {
	char* error_message = nullptr;
	if (LLVMVerifyModule(state.module, LLVMReturnStatusAction, &error_message) != 0) {
		std::cerr << "Generated module is invalid: " << error_message << std::endl;
		LLVMDisposeMessage(error_message);
		return false;
	}

	return true;
}

bool write_ir_file(ExampleState& state) {
	char* error_message = nullptr;
	if (LLVMPrintModuleToFile(state.module, state.ir_path.c_str(), &error_message) != 0) {
		std::cerr << "Failed to write IR file '" << state.ir_path << "': "
							<< error_message << std::endl;
		LLVMDisposeMessage(error_message);
		return false;
	}

	return true;
}

bool write_object_file(ExampleState& state) {
	char* error_message = nullptr;
	if (LLVMTargetMachineEmitToFile(
				state.target_machine,
				state.module,
				const_cast<char*>(state.object_path.c_str()),
				LLVMObjectFile,
				&error_message) != 0) {
		std::cerr << "Failed to write object file '" << state.object_path << "': "
							<< error_message << std::endl;
		LLVMDisposeMessage(error_message);
		return false;

	}

	return true;
}

bool link_executable(const ExampleState& state) {
	#ifdef _WIN32
	std::string command = "clang " + shell_quote(state.object_path) + " -o " +
			shell_quote(state.executable_path);
	#else
	std::string command = "cc " + shell_quote(state.object_path) + " -o " +
			shell_quote(state.executable_path);
	#endif
	int exit_code = std::system(command.c_str());
	if (exit_code != 0) {
		std::cerr << "Failed to link executable with command: " << command << std::endl;
		return false;
	}

	return true;
}

void dispose_state(ExampleState& state) {
	if (state.builder != nullptr) {
		LLVMDisposeBuilder(state.builder);
	}
	if (state.module != nullptr) {
		LLVMDisposeModule(state.module);
	}
	if (state.context != nullptr) {
		LLVMContextDispose(state.context);
	}
	if (state.target_machine != nullptr) {
		LLVMDisposeTargetMachine(state.target_machine);
	}
	if (state.target_triple != nullptr) {
		LLVMDisposeMessage(state.target_triple);
	}
}

int main(int argc, char** argv) {
	ExampleState state;

	initialize_llvm();
	configure_output_paths(state, argc, argv);
	create_module_state(state);

	if (!configure_target_machine(state)) {
		dispose_state(state);
		return 1;
	}

	build_ir(state);

	if (!verify_ir(state)) {
		dispose_state(state);
		return 1;
	}

	if (!write_ir_file(state)) {
		dispose_state(state);
		return 1;
	}

	if (!write_object_file(state)) {
		dispose_state(state);
		return 1;
	}

	if (!link_executable(state)) {
		dispose_state(state);
		return 1;
	}

	std::cout << "Wrote LLVM IR to " << state.ir_path << std::endl;
	std::cout << "Wrote object file to " << state.object_path << std::endl;
	std::cout << "Linked executable to " << state.executable_path << std::endl;
	dispose_state(state);
	return 0;
}
