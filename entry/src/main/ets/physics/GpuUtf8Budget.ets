// Exact UTF-8 length (including replacement of an unmatched UTF-16 surrogate).
export function utf8Bytes(text:string):number {
 let bytes:number=0;
 for(let i:number=0;i<text.length;i++){const c:number=text.charCodeAt(i);if(c<128)bytes++;else if(c<2048)bytes+=2;else if(c>=55296&&c<=56319&&i+1<text.length&&text.charCodeAt(i+1)>=56320&&text.charCodeAt(i+1)<=57343){bytes+=4;i++;}else bytes+=3;}
 return bytes;
}
