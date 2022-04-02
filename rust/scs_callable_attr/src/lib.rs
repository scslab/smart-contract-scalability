
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

use proc_macro::TokenStream;

use sha2::Sha256;

use sha2::Digest;


#[proc_macro_attribute]
pub fn scs_public_function(attr: TokenStream, item: TokenStream) -> TokenStream {
    let func : ItemFn = parse_macro_input!(item as ItemFn);

    let sig = &func.sig;

    let name = sig.ident.to_string();


    if sig.inputs.len() != 0
    {
        let res = quote!{
            compile_error!("haven't come up with a standard ABI for arg encoding/serialization yet, so no args on fns (call get_calldata manually)");
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
            fn #m_write ()
            {
              
              #call_original ();
            }
        };

        TokenStream::from(res)
    }
}
