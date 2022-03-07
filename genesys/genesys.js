"use strict"

class Interpreter {
   constructor() {
      this.reset()
   }

   //
   reset() {
      this._regs = []
      this._regs.length = 10
      this._regs.fill(0)
      this._mem = []
      this._exec = 0
      this._halt = false   
   }

   //
   setNextMemory(num) {
      this._mem.push(num)
   }

   //
   run() {
      let qntInstr = 0
      while (!this._halt && (this._exec < this._mem.length)) { // let be safe still
         let instr = this._mem[this._exec++]
         try {
            this.execute(instr)
         }
         catch (err) {
            this.printError(err, instr, this._exec - 1)
            this._halt = true
         }
         qntInstr++
      }
      return qntInstr   
   }

   //
   printError(err, instr, address) {
      console.log(`error: ${err} executing instr ${instr} at address: ${address}`)
   }

   //
   execute(instr) {
      // parse instruction
      const p2 = instr % 10
      let tmp = Math.floor(instr/10)
      const p1 = tmp % 10
      const icod = Math.floor(tmp / 10)

      switch (icod) {
      case 1: // 100 means halt
         // both params should be 0 // or we shell ignore their values
         if (p2 || p1)
            throw "invalid instruction" // ??? 
         this._halt = true
         break

      case 2: // 2dn means set register d to n (between 0 and 9)
         this._regs[p1] = p2
         break

      case 3: // 3dn means add n to register d
         tmp = this._regs[p1] + p2
         this._regs[p1] = tmp % 1000
         break

      case 4: // 4dn means multiply register d by n
         tmp = this._regs[p1] * p2
         this._regs[p1] = tmp % 1000
         break

      case 5: // 5ds means set register d to the value of register s
         this._regs[p1] = this._regs[p2]
         break

      case 6: // 6ds means add the value of register s to register d
         tmp = this._regs[p1] + this._regs[p2]
         this._regs[p1] = tmp % 1000
         break

      case 7: // 7ds means multiply register d by the value of register s
         tmp = this._regs[p1] * this._regs[p2]
         this._regs[p1] = tmp % 1000
         break

      case 8: // 8da means set register d to the value in RAM whose address is in register a
         this._regs[p1] = this._mem[this._regs[p2]]
         break

      case 9: // 9sa means set the value in RAM whose address is in register a to the value of register s
         this._mem[this._regs[p2]] = this._regs[p1]
         break

      case 0: // 0ds means goto the location in register d unless register s contains 0
         if (this._regs[p2])
            this._exec = this._regs[p1]
         break
      }
   }
}

//
async function main() {
   // initialize interpreter
   const mach = new Interpreter()

   // read standard input line by line 
   const readline = require('readline');
   const rl = readline.createInterface({
      input: process.stdin,
      output: process.stdout,
      terminal: false
   })

   const ReadCasesState = {ReadCasesQty: 0, ExpectEmptyLineBeforeFirstSet: 1, ReadInstr : 2}
   let cntCases = 0
   let caseNumber = 0
   let state = ReadCasesState.ReadCasesQty
   for await (const line of rl) {
      switch(state) {
      case ReadCasesState.ReadCasesQty: // read number of cases
         cntCases = parseInt(line)
         state = ReadCasesState.ExpectEmptyLineBeforeFirstSet
         break

      // for the first case there  should be empty line between number of cases and beginning of case
      // however in the example there is no such line
      // let's tolerate for this
      case ReadCasesState.ExpectEmptyLineBeforeFirstSet:
         if(line.length) {
            const instr = parseInt(line)
            mach.setNextMemory(instr)
         } 
         state = ReadCasesState.ReadInstr
         break

      case ReadCasesState.ReadInstr:
         if(line.length) {
            const instr = parseInt(line)
            mach.setNextMemory(instr)
         }
         else {   
               // no more instructions - let's execute the case
            const res = mach.run()
            process.stdout.write(`${res}\n`)
            caseNumber++
            mach.reset()
            if(caseNumber == cntCases)
               return   
         }
         break      
      }  
   }
}

main()
