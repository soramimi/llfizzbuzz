
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_os_ostream.h>

#include <stdio.h>

#pragma GCC diagnostic ignored "-Wswitch"

using namespace llvm;

class LLFizzBuzz {
private:
	Module *module;
	Function *current_function;
	DataLayout *data_layout;
	Type *int32ty;
	Function *func_printf;
	Function *func_putchar;
	Function *func_print_number;
	Function *func_print_fizz;
	Function *func_print_buzz;
	Function *func_print_newline;

	// グローバルバイト列を作成
	static Constant *createGlobalByteArrayPtr(Module *module, void const *p, size_t n)
	{
		LLVMContext &cx = module->getContext();
		Constant *data = ConstantDataArray::get(cx, ArrayRef<uint8_t>((uint8_t const *)p, n)); // 配列定数
		GlobalVariable *gv = new GlobalVariable(*module, data->getType(), true, Function::ExternalLinkage, data); // グローバル変数
		Value *i32zero = ConstantInt::get(Type::getInt32Ty(cx), 0); // 整数のゼロ
#if 0
        std::vector<Value *> zero2 = { i32zero, i32zero }; // ゼロふたつ
        return ConstantExpr::getInBoundsGetElementPtr(gv, zero2); // 実体を指すポインタの定数オブジェクトにする
#else
        return ConstantExpr::getInBoundsGetElementPtr(gv->getValueType(), gv, { i32zero, i32zero }); // LLVMのバージョンによってはこちら
#endif
	}

	// グローバル文字列を作成
	static Constant *createGlobalStringPtr(Module *module, StringRef str)
	{
		return createGlobalByteArrayPtr(module, str.data(), str.size() + 1); // ヌル文字を含めたバイト列を作成する
	}

	// print_number関数を作成
	Function *create_print_number_func(Module *module)
	{
		LLVMContext &cx = module->getContext();

		// printf関数を宣言

		// print_number関数を作成（戻り値なし、引数int）
		Function *func = Function::Create(FunctionType::get(Type::getVoidTy(cx), { Type::getInt32Ty(cx) }, false), Function::ExternalLinkage, "print_number", module);
		BasicBlock *block = BasicBlock::Create(cx, "entry", func);

		Value *arg1 = &*func->arg_begin(); // 最初の引数

		// printf関数を呼ぶ
		std::string str = "%d";
		Constant *arg0 = createGlobalStringPtr(module, str); // グローバル文字列を作成
		std::vector<Value *>args = { arg0, arg1 };
		CallInst::Create(func_printf, args, "", block); // printf(arg0, arg1) 呼び出し

		// return void
		ReturnInst::Create(cx, block);

		return func;
	}

	Function *create_print_string_func(Module *module, std::string str, std::string name)
	{
		LLVMContext &cx = module->getContext();

		// print_string関数を作成（戻り値なし、引数int）
		Function *func = Function::Create(FunctionType::get(Type::getVoidTy(cx), {}, false), Function::ExternalLinkage, name, module);
		BasicBlock *block = BasicBlock::Create(cx, "entry", func);

		// printf関数を呼ぶ
		Constant *arg0 = createGlobalStringPtr(module, str); // グローバル文字列を作成
		std::vector<Value *>args = { arg0 };
		CallInst::Create(func_printf, args, "", block); // printf(arg0, arg1) 呼び出し

		// return void
		ReturnInst::Create(cx, block);

		return func;
	}

	Function *create_print_newline_func(Module *module)
	{
		LLVMContext &cx = module->getContext();

		// print_newline関数を作成（戻り値なし、引数int）
		Function *func = Function::Create(FunctionType::get(Type::getVoidTy(cx), {}, false), Function::ExternalLinkage, "print_newline", module);
		BasicBlock *block = BasicBlock::Create(cx, "entry", func);

		// putchar関数を呼ぶ
		Value *v = ConstantInt::get(int32ty, (uint32_t)'\n');
		std::vector<Value *> args = { v };
		CallInst::Create(func_putchar, args, "", block); // putchar(arg0) 呼び出し

		// return void
		ReturnInst::Create(cx, block);

		return func;
	}

public:
	std::string generate()
	{
#if 0
        LLVMContext &cx = getGlobalContext();
#else
        LLVMContext cx;
#endif
		module = new Module("llfizzbuzz", cx);
		DataLayout dl(module);
		data_layout = &dl;

		int32ty = Type::getInt32Ty(cx);

		func_printf = Function::Create(FunctionType::get(int32ty, { Type::getInt8PtrTy(cx) }, true), Function::ExternalLinkage, "printf", module);
		func_putchar = Function::Create(FunctionType::get(int32ty, { int32ty }, true), Function::ExternalLinkage, "putchar", module);

		func_print_number = create_print_number_func(module);
		func_print_fizz = create_print_string_func(module, "Fizz", "print_fizz");
		func_print_buzz = create_print_string_func(module, "Buzz", "print_buzz");
		func_print_newline = create_print_newline_func(module);

		// main関数を作成（戻り値int、引数なし）
		current_function = Function::Create(FunctionType::get(int32ty, false), GlobalVariable::ExternalLinkage, "main", module);
		BasicBlock *entry_block = BasicBlock::Create(cx, "entry", current_function);

		// 定数
		Value *int_0 = ConstantInt::get(int32ty, (uint32_t)0);
		Value *int_1 = ConstantInt::get(int32ty, (uint32_t)1);
		Value *int_3 = ConstantInt::get(int32ty, (uint32_t)3);
		Value *int_5 = ConstantInt::get(int32ty, (uint32_t)5);
		Value *int_100 = ConstantInt::get(int32ty, (uint32_t)100);

		// 関数の内容を構築

		Value *loop_count_var = new AllocaInst(int32ty, "", entry_block); // ループカウンタを確保する
		unsigned int align = data_layout->getABITypeAlignment(int32ty);
		new StoreInst(int_0, loop_count_var, false, align, entry_block); // 代入する

		BasicBlock *cond_block = BasicBlock::Create(cx,"while.if", current_function); // 条件判定ブロック
		BasicBlock *body_block = BasicBlock::Create(cx,"while.body", current_function); // ループ本体ブロック
		BranchInst::Create(cond_block, entry_block);

		Value *loop_count_v = new LoadInst(loop_count_var, "", cond_block); // ループカウントの値を読み取る
		Value *loop_cond = new ICmpInst(*cond_block, ICmpInst::ICMP_SLT, loop_count_v, int_100, "cond"); // 100未満か

		loop_count_v = BinaryOperator::Create(BinaryOperator::Add, loop_count_v, int_1, "add", body_block); // 1足す
		new StoreInst(loop_count_v, loop_count_var, false, align, body_block); // ループカウントに代入する

		Value *mod3 = BinaryOperator::Create(BinaryOperator::SRem, loop_count_v, int_3, "", body_block); // 3で割った余り
		Value *mod5 = BinaryOperator::Create(BinaryOperator::SRem, loop_count_v, int_5, "", body_block); // 5で割った余り
		mod3 = new ICmpInst(*body_block, ICmpInst::ICMP_NE, mod3, int_0, ""); // boolに変換
		mod5 = new ICmpInst(*body_block, ICmpInst::ICMP_NE, mod5, int_0, ""); // boolに変換

		BasicBlock *print_number_block = BasicBlock::Create(cx,"print_number", current_function);
		BasicBlock *print_fizz_test_block = BasicBlock::Create(cx,"print_fizz_test", current_function);
		BasicBlock *print_fizz_block = BasicBlock::Create(cx,"print_fizz", current_function);
		BasicBlock *print_buzz_test_block = BasicBlock::Create(cx,"print_buzz_test", current_function);
		BasicBlock *print_buzz_block = BasicBlock::Create(cx,"print_buzz", current_function);
		BasicBlock *print_done_block = BasicBlock::Create(cx,"body_done_block", current_function);

		Value *v = BinaryOperator::Create(BinaryOperator::And, mod3, mod5, "", body_block); // 両方割り切れてないなら数値表示へ
		BranchInst::Create(print_number_block, print_fizz_test_block, v, body_block); // 割り切れているならFizzテストへ

		BranchInst::Create(print_buzz_test_block, print_fizz_block, mod3, print_fizz_test_block); // 3で割り切れるならFizz表示。そうでないならBuzzテストへ
		BranchInst::Create(print_done_block, print_buzz_block, mod5, print_buzz_test_block); // 5で割り切れるならBuzz表示。そうでないなら表示完了へ

		// 数値表示ブロック
		std::vector<Value *> args = { loop_count_v };
		CallInst::Create(func_print_number, args, "", print_number_block); // print_number関数を実行
		BranchInst::Create(print_done_block, print_number_block); // 表示完了へ

		// Fizz表示ブロック
		CallInst::Create(func_print_fizz, {}, "", print_fizz_block); // print_fizz関数を実行
		BranchInst::Create(print_buzz_test_block, print_fizz_block); // 表示完了へ

		// Buzz表示ブロック
		CallInst::Create(func_print_buzz, {}, "", print_buzz_block); // print_buzz関数を実行
		BranchInst::Create(print_done_block, print_buzz_block); // 表示完了へ

		// 表示完了ブロック
		CallInst::Create(func_print_newline, {}, "", print_done_block); // 改行出力
		BranchInst::Create(cond_block, print_done_block); // ループの最初（条件判定ブロック）へ

		// ループ終了ブロック
		BasicBlock *loop_end_block = BasicBlock::Create(cx,"while.exit", current_function);

		// 条件判定ブロックに分岐命令を追加
		BranchInst::Create(body_block, loop_end_block, loop_cond, cond_block);

		// return 0
		ReturnInst::Create(cx, int_0, loop_end_block);

		// LLVM IR を出力
		std::string ll;
		raw_string_ostream o(ll);
		module->print(o, nullptr);
		o.flush();
		return ll;
	}
};

int main()
{
	LLFizzBuzz ll;
	std::string llvm_ir = ll.generate();
	fwrite(llvm_ir.c_str(), 1, llvm_ir.size(), stdout);

	return 0;
}
