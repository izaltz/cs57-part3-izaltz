
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <set>
#include <map>
#include <vector>
#include <string>
#include <cassert>
#include <cctype>

using namespace std;

#define prt(x) if(x) { printf("%s\n", x); }

/*************************** cseElimination() *****************************/
/* function that performs constant subexpression elimination as described in 
 * part3 assignment page */
bool cseElimination(LLVMBasicBlockRef block){
    printf("in cseElimination\n");
    int replaceCount = 0;  
    //go through all the instructions in a basic block  
    for(LLVMValueRef inst = LLVMGetFirstInstruction(block); inst != NULL; inst  = LLVMGetNextInstruction(inst)){
        //check that the instruction is not a terminator, store, alloca, or call
        if(LLVMIsATerminatorInst(inst)){
            continue;
        }
       
        LLVMOpcode opcode = LLVMGetInstructionOpcode(inst);
        if(LLVMIsAStoreInst(inst)){
            continue;
        }

        if(LLVMIsAAllocaInst(inst)){
            continue;
        }

        if(LLVMIsACallInst(inst)){
            continue;
        }


        LLVMValueRef potentialMatch; //variable that keeps track of instruction if it seems to match initial instruction
        for(LLVMValueRef inst_comp = LLVMGetNextInstruction(inst); inst_comp != NULL; inst_comp = LLVMGetNextInstruction(inst_comp)){
            //go through the rest of the instruction in the block to compare to initial int
            LLVMOpcode opcode_comp = LLVMGetInstructionOpcode(inst_comp);
            
            //if opcodes don't match, move to the next inst
            if (opcode != opcode_comp){
                continue;
            }

            int numOps = LLVMGetNumOperands(inst);
            int numOps_comp = LLVMGetNumOperands(inst_comp);
            
            //if the number of operands don't match, move to the next inst
            if (numOps != numOps_comp){
                continue;
            } else {
                //if the number of operands match, compare each pair
                int numMatches = 0;
                for (int i = 0; i < numOps; i++){
                    LLVMValueRef operand = LLVMGetOperand(inst, i);
                    LLVMValueRef operand_comp = LLVMGetOperand(inst_comp, i);
                    if (operand == operand_comp){
                        numMatches++;
                    } 
                }
                //if the number of matching pairs does not match the total pairs, move to next inst
                if (numMatches != numOps){
                    continue;
                }
            }

            //if operands and opcodes match, mark inst_comp as a potential match
            potentialMatch = inst_comp;
            bool toReplace = true;
            //if inst is a load, check all the instructions between it and the potential match
            if (LLVMIsALoadInst(inst)){ 
                for (LLVMValueRef inst_between = LLVMGetNextInstruction(inst); inst_between != potentialMatch; inst_between = LLVMGetNextInstruction(inst_between)) {
                    //if an inst_between is a store, check that it does not modify the operand the load inst modifies
                    if (LLVMIsAStoreInst(inst_between)){
                        if(LLVMGetOperand(inst_between,1) == LLVMGetOperand(inst, 0)){
                            toReplace = false;
                        }
                    }
                }
            }

            //if potential match is a match, replace it with inst
            if(toReplace){
                LLVMReplaceAllUsesWith(potentialMatch, inst);
                replaceCount++;
            }
    
        }
    }

    printf("done with cseElimination\n");
    if(replaceCount == 0){
            printf("cse returned false\n");
    } else {
        printf("cse returned true\n");
    }
    printf("BB post CSE:\n");
    for(LLVMValueRef inst = LLVMGetFirstInstruction(block); inst != NULL; inst = LLVMGetNextInstruction(inst)){
        LLVMDumpValue(inst);
        printf("\n");
    }

    //return boolean if change to block occured
    if (replaceCount == 0){
        return false;
    } else {
        return true;
    }
}


/*************************** deadCodeElimination() *****************************/
/* function that performs deadCode elimination as described in 
 * part3 assignment page */
bool deadCodeElimination(LLVMBasicBlockRef block){
    std::vector<LLVMValueRef> toDeleteList; //vector to store insts to be deleted
    bool toDelete = false; 
    //go through all instructions in block
    for(LLVMValueRef inst = LLVMGetFirstInstruction(block); inst!= NULL; inst = LLVMGetNextInstruction(inst)){
        LLVMOpcode opcode = LLVMGetInstructionOpcode(inst);
        //check that instruction is not a store, terminator, alloca, or call
        if (LLVMIsAStoreInst(inst)){
            continue;
        }
        if (LLVMIsATerminatorInst(inst)){
            continue;
        }
        if (LLVMIsAAllocaInst(inst)){
            continue;
        }
        if (LLVMIsACallInst(inst)){
            continue;
        }
        //check that the instruction is used
        if (LLVMGetFirstUse(inst) == NULL){
            //if not, mark it to be deleted
            toDelete = true;
            toDeleteList.push_back(inst);
        }
    }

    //go through deletion list and delete the marked instructions
    for(auto inst : toDeleteList){
        if (LLVMIsAStoreInst(inst)){
            continue;
        } if (LLVMIsAAllocaInst(inst)){
            continue;
        } if (LLVMIsACallInst(inst)){
            continue;
        } if (LLVMIsATerminatorInst(inst)){
            continue;
        }
        LLVMInstructionEraseFromParent(inst);
    }

    printf("done with deadCodeElimination\n");
    if(toDelete == true){
            printf("dce returned true\n");
        } else {
            printf("dce returned false\n");
        }
        printf("BB post DCE:\n");
        for(LLVMValueRef inst = LLVMGetFirstInstruction(block); inst != NULL; inst = LLVMGetNextInstruction(inst)){
             LLVMDumpValue(inst);
             printf("\n");

        }
    //boolean reflecting if deletions occurred
    return toDelete;
}

/*************************** constantFolding() *****************************/
/* function that performs constant folding as described in 
 * part3 assignment page */
bool constantFolding(LLVMBasicBlockRef block){
    printf("in constantFolding\n");
    int numChanges = 0; //keep track of number of changes made to BB
    std::vector<LLVMValueRef> toDeleteList;
    //go through all the instructions in the block
    for (LLVMValueRef inst = LLVMGetFirstInstruction(block); inst != NULL; inst = LLVMGetNextInstruction(inst)){
        LLVMOpcode opcode = LLVMGetInstructionOpcode(inst);
        //check that the instruction is an add, sub, or multiplication instruction
        if(opcode == LLVMAdd || opcode == LLVMSub || opcode == LLVMMul){
            //get the operands on each side of the expression
            LLVMValueRef op1 = LLVMGetOperand(inst, 0);
            LLVMValueRef op2 = LLVMGetOperand(inst, 1);

            //check that they are both constants
            if (LLVMIsAConstant(op1) && LLVMIsAConstant(op2)){
                LLVMValueRef result;

                //compute the result of the expression
                if(opcode == LLVMAdd){
                    result = LLVMConstAdd(op1, op2);
                } else if (opcode == LLVMSub){
                    result = LLVMConstSub(op1, op2);
                } else {
                    result = LLVMConstMul(op1, op2);
                }

                //replace all uses of the instruction with the result
                LLVMReplaceAllUsesWith(inst, result);

                //delete the original instruction
                toDeleteList.push_back(inst);
                numChanges++;
            }

        }
    }
    //delete the instructions
    for(auto inst : toDeleteList){
        LLVMInstructionEraseFromParent(inst);
    }

    //debug prings
    printf("done with constantFolding\n");
    if(numChanges>0){
        printf("cf returned true\n");
    } else {
        printf("cf returned false\n");
    }
        printf("BB post CF:\n");
    for(LLVMValueRef inst = LLVMGetFirstInstruction(block); inst != NULL; inst = LLVMGetNextInstruction(inst)){
        LLVMDumpValue(inst);
        printf("\n");
    }

    //return boolean reflecting if changes were made
    if (numChanges > 0){
        return true;
    } else {
        return false;
    }
}

/*************************** genSet() *****************************/
/* function that accepts a basic block and creates a gen set, as described in 
 * part3 assignment page */
std::set<LLVMValueRef> makeGenSet(LLVMBasicBlockRef block){

    std::set<LLVMValueRef> gen_B; //initialize empty set
    std::vector<LLVMValueRef> toDeleteList;
    //loop through all the instruction in set
    for (LLVMValueRef inst = LLVMGetFirstInstruction(block); inst != NULL; inst = LLVMGetNextInstruction(inst)){
        
        //check if inst is a store
        if (LLVMIsAStoreInst(inst)){
            LLVMValueRef operand = LLVMGetOperand(inst, 1);
            //if there are instructions in the gen set, loop through them
            for(auto inst_comp : gen_B){

                //if any of the instructions is killed by the store
                LLVMValueRef operand_comp = LLVMGetOperand(inst_comp, 1);
                if (operand == operand_comp){
                   
                    //delete from set
                    toDeleteList.push_back(inst_comp);
                }
            }
            //insert the store into the set
            for (auto instToDelete : toDeleteList){
                gen_B.erase(instToDelete);
            }
            gen_B.insert(inst);
        }

    }

    //return the set
    return gen_B;
}

/*************************** allStoresSet() *****************************/
/* function that accepts an LLVM func and creates a set of all store instructions */

std::set<LLVMValueRef> allStoresSet(LLVMValueRef func){
    std::set<LLVMValueRef> allStores; //initialize an empty set

    //loop through every block in func
    for (LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(func); block != NULL; block = LLVMGetNextBasicBlock(block)){
        //loop through every instruction in block
        for (LLVMValueRef inst = LLVMGetFirstInstruction(block); inst!=NULL; inst = LLVMGetNextInstruction(inst)){
            //if the instruction is a store
            if (LLVMIsAStoreInst(inst)){
                //insert it into the set
                allStores.insert(inst);
            }
        }
    }
    //return a set of all stores in the function
    return allStores;
}

/*************************** killSet() *****************************/
/* function that accepts a basic block and creates a kill set, as described in 
 * part3 assignment page */
std::set<LLVMValueRef> makeKillSet(LLVMBasicBlockRef block, std::set<LLVMValueRef> allStores){

    std::set<LLVMValueRef> kill_B; //initialize an empty set
    //loop through every instruction in the block
    for(LLVMValueRef inst = LLVMGetFirstInstruction(block); inst != NULL; inst = LLVMGetNextInstruction(inst)){
       
        //if the instruction is a store
        if(LLVMIsAStoreInst(inst)){
    
            LLVMValueRef operand = LLVMGetOperand(inst, 1);
            //see if it kills any of the store instructions in the function
            for (LLVMValueRef inst_comp : allStores){
               
                LLVMValueRef operand_comp = LLVMGetOperand(inst_comp, 1);
            
                if(operand == operand_comp){ 
                    //add the killed instructions to the kill set
                    if (inst_comp != inst){
                        kill_B.insert(inst_comp);
                    }
                }
            }
        }
    }
    //return kill set
    return kill_B;
}

/*************************** unionFunc() *****************************/
/* unites two sets by combining their values and removing duplicate values */
std::set<LLVMValueRef> unionFunc(std::set<LLVMValueRef> set1, std::set<LLVMValueRef> set2){

    std::set<LLVMValueRef> resultSet;
    resultSet = set1;
    for (auto value : set2){
        resultSet.insert(value);
    }
    return resultSet;
}

/*************************** subtractFunc() *****************************/
/* accepts two sets and produces a set with the unique values within the sets */
std::set<LLVMValueRef> subtractFunc(std::set<LLVMValueRef> set1, std::set<LLVMValueRef> set2){
    std::set<LLVMValueRef> resultSet;
    for (auto value : set1){
        if(set2.find(value) == set2.end()){
            resultSet.insert(value);
        }
    }
    return resultSet;
}

/*************************** predsSet() *****************************/
/* function that accepts an LLVM function and creates a map of each basic block's predecessor blocks */
std::map<LLVMBasicBlockRef, std::set<LLVMBasicBlockRef>> predsSet(LLVMValueRef func){
    
    
    std::map<LLVMBasicBlockRef, std::set<LLVMBasicBlockRef>> allPreds;
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(func); block != NULL; block = LLVMGetNextBasicBlock(block)){
        std::set<LLVMBasicBlockRef> preds_set;
        allPreds[block] = preds_set;
    }

    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(func); block != NULL; block = LLVMGetNextBasicBlock(block)){
        LLVMValueRef terminatorInst = LLVMGetBasicBlockTerminator(block);
    
        for(int i = 0; i < LLVMGetNumSuccessors(terminatorInst); i++){
    
            LLVMBasicBlockRef successorBlock = LLVMGetSuccessor(terminatorInst, i);
            allPreds.at(successorBlock).insert(block);
        }
    }

    return allPreds;

}

/*************************** constantPropHelper() *****************************/
/* helper function that conducts the constant propogation algorithm. Called by constantProp() */
bool constantPropHelper(std::map<LLVMBasicBlockRef, std::set<LLVMBasicBlockRef>> allPreds, LLVMValueRef func, std::map<LLVMBasicBlockRef, std::set<LLVMValueRef>> genSet, std::map<LLVMBasicBlockRef, std::set<LLVMValueRef>> killSet, std::set<LLVMValueRef> allStoresSet){
    
    bool changedCP = false;
    // initialize maps of in and out sets
    std::map<LLVMBasicBlockRef, std::set<LLVMValueRef>> inSet;
    std::map<LLVMBasicBlockRef, std::set<LLVMValueRef>> outSet;

    //go through each block in the function
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(func); block != NULL; block = LLVMGetNextBasicBlock(block)){
        //for each block, create an empty in set and add it to the map
        std::set<LLVMValueRef> in_B;
        inSet[block] = in_B;

        //for each block, get its gen set and make it its out set
        std::set<LLVMValueRef> out_B = genSet[block];
        outSet[block] = out_B;

    }

    //while changes continue to be made
    bool change = true;
    int i = 0;
    while(change == true){
        change = false;
        //go through each block in the function
        for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(func); block != NULL; block = LLVMGetNextBasicBlock(block)){
            //get the block's predecessors
            std::set<LLVMBasicBlockRef> preds_B = allPreds.at(block);
            
            //update the inset as the union of the outsets of its predecessors
            for (auto predBlock : preds_B){
                inSet[block] = unionFunc(inSet[block], outSet[predBlock]);
            }
        }
        //go through each block in the function
        for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(func); block != NULL; block = LLVMGetNextBasicBlock(block)){
            //save the old outset for comparison
            std::set<LLVMValueRef> oldOut = outSet[block];
            //update the outset
            outSet[block] = unionFunc(genSet[block], subtractFunc(inSet[block], killSet[block]));
            //compare it to the old out set and update if things changed
            if (outSet[block] != oldOut){
                change = true;
            }
        }
        i++;
    }
  
    //handle load and store instructions
    std::vector<LLVMValueRef> toDeleteList; //keeps track of inst to be deleted
    //go through each block in the function
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(func); block != NULL; block = LLVMGetNextBasicBlock(block)){
        //get the in set of each block
        std::set<LLVMValueRef> R = inSet[block];
        std::vector<LLVMValueRef> deleteFromR; //keeps track of inst to be deleted
        //go through each instruction in the block
        for(LLVMValueRef inst = LLVMGetFirstInstruction(block); inst != NULL; inst=LLVMGetNextInstruction(inst)){
            
            //if its a store instruction
            if(LLVMIsAStoreInst(inst)){
                //get its second operand
                LLVMValueRef operand = LLVMGetOperand(inst, 1);
                //loop through the in set
                for(LLVMValueRef inst_comp : R){
                    if(LLVMIsAStoreInst(inst_comp)){
                        //compare the operand with the in set instruction operand
                        LLVMValueRef operand_comp = LLVMGetOperand(inst_comp, 1);
                        if (operand == operand_comp){
                            //delete the instruction from the in set
                            deleteFromR.push_back(inst_comp);
                        }
                    }
                }
                //insert the new instruction into the set
                R.insert(inst);
                //delete the instruction that were marked for deletion
                for (auto inst : deleteFromR){
                    R.erase(inst);
                }
            }

            //if the instruction is a load
            if (LLVMIsALoadInst(inst)){
                    //get the value %t that it loads from
                    LLVMValueRef pointer = LLVMGetOperand(inst, 0);
                    std::set<LLVMValueRef> matching_operand_stores;
                    //loop through the instructions in the in set
                    for(auto inst_comp : R){
                        //get the operand of the instruction and compare
                        if(LLVMGetOperand(inst_comp, 1) == pointer){
                            //insert it into the set of potential matches
                            matching_operand_stores.insert(inst_comp);
                        }
                    }

                    int constant;
                    bool gettingFirstConst = true;
                    bool canChange = true;
                    //loop through the potential matches
                    for(auto store_inst : matching_operand_stores){
                        //check that the operand is a constant
                        LLVMValueRef potentialConst = LLVMGetOperand(store_inst, 0);
                        if (!LLVMIsAConstantInt(potentialConst)){
                            canChange = false;
                            break;
                        }
                        //during the first inst in the loop, get the constant and convert to SE value to compare
                        if (gettingFirstConst){
                            constant = LLVMConstIntGetSExtValue(potentialConst);
                            gettingFirstConst = false;
                            continue;
                        }
                        //for the rest of the insts compare their operands to the constant
                        if(constant != LLVMConstIntGetSExtValue(potentialConst)){
                            //if they don't match, exit the loop
                            canChange = false;
                            break;
                        }
                    }

                    //if matches were found
                    if (canChange){
                        //mark the load instruction for deletion
                        toDeleteList.push_back(inst);
                        //replace it with the store instruction with the matching constant
                        LLVMReplaceAllUsesWith(inst, LLVMConstInt(LLVMInt32Type(), constant, 1));
                        changedCP = true;
                    }
            }
        }
    }

    //delete instructions that were marked for deletion
    for (auto toDeleteInst : toDeleteList){
        if (LLVMIsAStoreInst(toDeleteInst)){
            continue;
        } if (LLVMIsAAllocaInst(toDeleteInst)){
            continue;
        } if (LLVMIsACallInst(toDeleteInst)){
            continue;
        } if (LLVMIsATerminatorInst(toDeleteInst)){
            continue;
        }
        LLVMInstructionEraseFromParent(toDeleteInst);
    }

    printf("done with helper\n");
    return changedCP; //return boolean reflecting if changes were made
    
}

/*************************** constantProp() *****************************/
/* Function that accepts an LLVM function, creates gen, kill, predecessor, and store sets for all blocks in that function,
 * and calls constantPropHelper() to do constant propogation */
bool constantProp(LLVMValueRef func){
    
    //initialize map of gen sets
    std::map<LLVMBasicBlockRef, std::set<LLVMValueRef>> genSet;
    //loop through each block in the function
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(func); block != NULL; block = LLVMGetNextBasicBlock(block)) {
        //for each block, create a genSet and add it to the map
        std::set<LLVMValueRef> gen_B = makeGenSet(block);
        genSet[block] = gen_B;
    }

    //debug print
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(func); block != NULL; block = LLVMGetNextBasicBlock(block)){
        for (LLVMValueRef inst = LLVMGetFirstInstruction(block); inst != NULL; inst = LLVMGetNextInstruction(inst)){
            LLVMDumpValue(inst);
            printf("\n");
        }
        std::set<LLVMValueRef> set = genSet.at(block);
        if(set.empty()){
            printf("The set is empty\n");
        } else {
            printf("The set is of size %ld: \n", set.size());
        }
        for(auto in : set){
            LLVMDumpValue(in);
            printf("\n");
        }
    }

    //get a set of all store insts in the function
    std::set<LLVMValueRef> allStores = allStoresSet(func);

    //initialize map of gen sets
    std::map<LLVMBasicBlockRef, std::set<LLVMValueRef>> killSet;
    //go through every block in the function
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(func); block != NULL; block = LLVMGetNextBasicBlock(block)) {
        //for each block, create a kill set and add it to the map
        std::set<LLVMValueRef> kill_B = makeKillSet(block, allStores);
        killSet[block] = kill_B;
    }

    //debug print
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(func); block != NULL; block = LLVMGetNextBasicBlock(block)){
        printf("Got Basic Block in kill:\n");
        for (LLVMValueRef inst = LLVMGetFirstInstruction(block); inst != NULL; inst = LLVMGetNextInstruction(inst)){
            LLVMDumpValue(inst);
            printf("\n");
        }
        std::set<LLVMValueRef> setK = killSet.at(block);
        if(setK.empty()){
            printf("The kill set is empty\n");
        } else {
            printf("The kill set is of size %ld: \n", setK.size());
        }
        for(auto in : setK){
            LLVMDumpValue(in);
            printf("\n");
        }
    }

    //create a map of predecessor sets for each block in the function
    std::map<LLVMBasicBlockRef, std::set<LLVMBasicBlockRef>> allPreds = predsSet(func);

    //debug print
    for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(func); block != NULL; block = LLVMGetNextBasicBlock(block)){
        std::set<LLVMBasicBlockRef> predB = allPreds.at(block);
        printf("pred set size: %ld\n", predB.size());
        for(auto block : predB){
            for(LLVMValueRef inst = LLVMGetFirstInstruction(block); inst != NULL; inst = LLVMGetNextInstruction(inst)){
                LLVMDumpValue(inst);
                printf("\n");
            }
            printf("\n");
        }
    }

    //do constant propogation
    bool changedCP = false;
    changedCP = constantPropHelper(allPreds, func, genSet, killSet, allStores);
    
    //return the boolean reflecting if changes were made in constant propogation
    return changedCP;

}

/*************************** createLLVMModel() *****************************/
/* adapted from lecture9 llvm_parcer.c */
LLVMModuleRef createLLVMModel(char * filename){
	char *err = 0;

	LLVMMemoryBufferRef ll_f = 0;
	LLVMModuleRef m = 0;

	LLVMCreateMemoryBufferWithContentsOfFile(filename, &ll_f, &err);

	if (err != NULL) { 
		prt(err);
		return NULL;
	}
	
	LLVMParseIRInContext(LLVMGetGlobalContext(), ll_f, &m, &err);

	if (err != NULL) {
		prt(err);
	}

	return m;
}

#define prt(x) if(x) { printf("%s\n", x); }

/*************************** walkBasicblocks() *****************************/
/* adapted from lecture9 llvm_parcer.c */
void walkBasicblocks(LLVMValueRef function){
	for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
 			basicBlock;
  			basicBlock = LLVMGetNextBasicBlock(basicBlock)) {
		
		printf("In basic block\n");
        printf("BB pre CSE:\n");
        for(LLVMValueRef inst = LLVMGetFirstInstruction(basicBlock); inst != NULL; inst = LLVMGetNextInstruction(inst)){
             LLVMDumpValue(inst);
             printf("\n");
        }
        //calls local optimization
        bool cseChange = cseElimination(basicBlock);
        bool dceChange = deadCodeElimination(basicBlock);
        bool cfChange = constantFolding(basicBlock);
        if(cfChange == true){
            printf("cf returned true\n");
        } else {
            printf("cf returned false\n");
        }
        printf("BB post CF:\n");
        for(LLVMValueRef inst = LLVMGetFirstInstruction(basicBlock); inst != NULL; inst = LLVMGetNextInstruction(inst)){
             LLVMDumpValue(inst);
             printf("\n");

        }
        //continues to executed local optimizations while changes are still being made
        while (cseChange == true || dceChange == true || cfChange == true){
            cseChange = cseElimination(basicBlock);
            dceChange = deadCodeElimination(basicBlock);
            cfChange = constantFolding(basicBlock);
        }
	}
}


/*************************** walkFunctions() *****************************/
/* adapted from lecture9 llvm_parcer.c */
void walkFunctions(LLVMModuleRef module){
    printf("in walkFunctions\n");
	for (LLVMValueRef function =  LLVMGetFirstFunction(module); 
			function; 
			function = LLVMGetNextFunction(function)) {

		const char* funcName = LLVMGetValueName(function);	
		printf("Function Name: %s\n", funcName);
		walkBasicblocks(function);
        printf("done with local optimizations\n");
        bool changedCP = constantProp(function);
        bool changedCF;
        //continue to call constant propogation and constant folding while changes are still being made
        if(changedCP = true){

            for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(function); block != NULL; block = LLVMGetNextBasicBlock(block)){
                changedCF = constantFolding(block);
            }
            while(changedCP == true || changedCF == true){
                changedCP = constantProp(function);
                for(LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(function); block != NULL; block = LLVMGetNextBasicBlock(block)){
                    changedCF = constantFolding(block);
            }
            }
        }
    }
}


/*************************** walkGlobalValues() *****************************/
/* adapted from lecture9 llvm_parcer.c */
void walkGlobalValues(LLVMModuleRef module){
	for (LLVMValueRef gVal =  LLVMGetFirstGlobal(module);
                        gVal;
                        gVal = LLVMGetNextGlobal(gVal)) {

                const char* gName = LLVMGetValueName(gVal);
                printf("Global variable name: %s\n", gName);
        }
}


/*************************** main() *****************************/
/* main function for optimizations */
int main(int argc, char** argv)
{
    printf("in main\n");
	LLVMModuleRef m;

	if (argc == 2){
		m = createLLVMModel(argv[1]);
	}
	else{
		m = NULL;
		return 1;
	}

	if (m != NULL){
        //call functions in order to optimize the module
        printf("got module\n");
        walkGlobalValues(m);
		walkFunctions(m);
		LLVMPrintModuleToFile (m, "test_new.ll", NULL);
        LLVMDisposeModule(m);
	}
	else {
	    fprintf(stderr, "m is NULL\n");
	}
    //clean up
    LLVMShutdown();
	
	return 0;
}
