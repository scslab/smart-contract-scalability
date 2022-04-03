#[macro_use]
extern crate syn;
#[macro_use]
extern crate quote;
#[macro_use]
extern crate sha2;
#[macro_use]
extern crate hex;

use syn::parse::{Parse, ParseStream};
use syn::ItemFn;
use syn::LitStr;

use proc_macro::TokenStream;

use sha2::Sha256;

use sha2::Digest;

fn copy_slice (buf : &[u8]) -> [u8 ; 4]
{
    let mut out : [u8; 4] = [0 ; 4];
    out[0] = buf[0];

    out[1] = buf[1];
    out[2] = buf[2];
    out[3] = buf[3];

    out
}

#[proc_macro]
pub fn method_id(item : TokenStream) -> TokenStream {

    let string : LitStr = parse_macro_input!(item as LitStr);

    let mut hasher = Sha256::new();
    hasher.update(string.value().as_bytes());
    let result = hasher.finalize();

    let buf = copy_slice(&result);

    let out : i32 = i32::from_le_bytes(buf);

    TokenStream::from(
        quote!{
            #out
        }
    )
}

#[proc_macro_attribute]
pub fn scs_public_function(attr: TokenStream, item: TokenStream) -> TokenStream {
    let func : ItemFn = parse_macro_input!(item as ItemFn);

    let sig = &func.sig;

    let name = sig.ident.to_string();


    if sig.inputs.len() != 1
    {
        let res = quote!{
            compile_error!("haven't hooked up a backed data API yet, so just calldata should have size 1 (calldata_len : i32)");
        };
        TokenStream::from(res)
    }
    else
    {
        let mut hasher = Sha256::new();
        hasher.update(name.as_bytes());
        let result = hasher.finalize();

        let prefix = &result[0..4];

        let hex = hex::encode_upper(&prefix);

        let mut methodname_out = "pub".to_string();
        methodname_out.push_str(&hex);

        let m_write = format_ident!("{}", methodname_out);

        // the following line induces a panic in rustc
        //let call_original = format_ident!("{}();", name);
        let call_original = format_ident!("{}", name);
        let no_mangle = quote!{#[no_mangle]};
        let always_inline = quote!{#[inline(always)]};

        let res = quote!{
            #always_inline
            #func

            #no_mangle
            fn #m_write ( calldata_len : i32)
            { 
              #call_original (calldata_len);
            }
        };

        TokenStream::from(res)
    }
}

