/**
 * Neuro-Silica MEMT1 fallback
 * Author: Lewis James Seals Jr. (ljrseals-dev)
 * Copyright (c) 2026 Lewis James Seals Jr.
 * SPDX-License-Identifier: MIT
 */
export const U64_MASK = (1n << 64n) - 1n;
function assertU64(v,n){if(typeof v!=="bigint"||v<0n||v>U64_MASK)throw new RangeError(n+" must be unsigned 64-bit BigInt");}
function assertAddress(v,n){if(!Number.isSafeInteger(v)||v<0)throw new RangeError(n+" must be a non-negative safe integer");}
export function cpuMatrixUpdate(mem,matrixAddr,gamma,omega,keyAddr,valAddr){
 // Compatibility adapter for the original callback API: (read64, write64, matrix, gamma, omega, key, val).
 if(typeof mem==="function"&&typeof matrixAddr==="function"){
  const read64=mem,write64=matrixAddr;mem={read64,write64};
  [matrixAddr,gamma,omega,keyAddr,valAddr]=[gamma,omega,keyAddr,valAddr,arguments[6]];
 }
 if(!mem||typeof mem.read64!=="function"||typeof mem.write64!=="function")throw new TypeError("mem must provide read64 and write64");
 for(const [v,n] of [[matrixAddr,"matrixAddr"],[keyAddr,"keyAddr"],[valAddr,"valAddr"]])assertAddress(v,n);
 assertU64(gamma,"gamma");assertU64(omega,"omega");
 for(let i=0;i<4;i++){const k=mem.read64(keyAddr+i*8);assertU64(k,"key["+i+"]");
  for(let j=0;j<4;j++){const v=mem.read64(valAddr+j*8);assertU64(v,"val["+j+"]");const a=matrixAddr+(i*4+j)*8;const m=mem.read64(a);assertU64(m,"matrix["+i+","+j+"]");
   const forget=((m*gamma)&U64_MASK)>>8n,write=((k*v*omega)&U64_MASK)>>8n;mem.write64(a,(forget+write)&U64_MASK);
  }
 }
}
