#[macro_use]
extern crate syn;
#[macro_use]
extern crate quote;

use syn::ItemFn;
use syn::LitStr;
use syn::FnArg::Typed;
use syn::Ident;

use syn::ReturnType::{Default, Type};

use proc_macro::TokenStream;

use sha2::Sha256;

use sha2::Digest;

use quote::ToTokens;

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


    if sig.inputs.len() >= 2
    {
        let res = quote!{
            compile_error!("currently only support at most one argument");
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

        if sig.inputs.len() == 0
        {

            let res = quote!{
                #always_inline
                #func

                #no_mangle
                fn #m_write ( calldata_len : i32)
                {
                    #call_original ();
                }
            };

            println!("output : {}", res);
            TokenStream::from(res)
        } else
        {
            let input = sig.inputs.first().expect("has input");
            let t = match input
            {
                Typed(pat_type) => pat_type,
                _ =>
                {
                    let res = quote!{
                        compile_error!("no self on smart contract methods");
                    };
                    return TokenStream::from(res);
                },
            };

            //let arg_type = format_type!("{}", (*t.ty).to_token_stream().to_string());
            let arg_type = &*t.ty;

            let backed_type_in = quote!(
                ::scs_sdk::call_argument::BackedType::<#arg_type, {core::mem::size_of::<#arg_type>()}>
            );

            let res = quote!{
                #always_inline
                #func

                #no_mangle
                fn #m_write ( calldata_len : i32)
                {
                   let arg = #backed_type_in :: new_from_calldata(calldata_len);
                    #call_original (&arg.get());
                }
            };
            println!("output : {}", res);
            TokenStream::from(res)
        }
    }
}

#[proc_macro_attribute]
pub fn scs_interface_method(attr: TokenStream, item: TokenStream) -> TokenStream {
    let func : ItemFn = parse_macro_input!(item as ItemFn);

    let sig = &func.sig;

    let fn_name = sig.ident.to_string();

    if sig.inputs.len() >= 2
    {
        let res = quote!{
            compile_error!("currently only support at most one argument");
        };
        return TokenStream::from(res);
    }

    let packagename : Ident = parse_macro_input!(attr as Ident);

    let mut backed_type_in = quote!(
        compile_error!("can't use this")
    );

    let args_flag = sig.inputs.len() != 0;

    if args_flag
    {
        let input = sig.inputs.first().expect("has input");
        let t = match input
        {
            Typed(pat_type) => pat_type,
            _ =>
            {
                let res = quote!{
                    compile_error!("no self on smart contract methods");
                };
                return TokenStream::from(res);
            },
        };

        let arg_type = &*t.ty;//format_ident!("{}", (*t.ty).to_token_stream().to_string());
        backed_type_in = quote!(
            ::scs_sdk::call_argument::BackedType::<#arg_type, {core::mem::size_of::<#arg_type>()}>
        );
    }


    let original_fn_name = format_ident!("{}", fn_name);

    let has_ret = match sig.output {
        Default => false,
        _ => true,
    };

    //TODO
    let mut backed_type_ret =         
        quote!{
            ::scs_sdk::call_argument::BackedType::<(), 0>
        };
    if has_ret
    {
        let ret_type = match &sig.output
        {
            Default =>
            {
                let res = quote!{
                    compile_error!("impossible");
                };
                return TokenStream::from(res);
            },
            Type(_, t) =>
            {
                format_ident!("{}", (*t).to_token_stream().to_string())
            },
        };

        backed_type_ret = quote!{
             ::scs_sdk::call_argument::BackedType::<#ret_type, {core::mem::size_of::<#ret_type>()}>
        };
    }


    let output = 

    if args_flag && has_ret
    {
        quote!{

            #func

            impl <'a> #packagename <'a>
            {
                pub fn #original_fn_name (&self, arg : &#backed_type_in) -> #backed_type_ret
                {
                    let out : #backed_type_ret = self.proxy.call(method_id!(#fn_name), &arg);
                    out
                }
            }
        }
    } else if args_flag && !has_ret
    {
        quote!{

            #func

            impl <'a> #packagename <'a>
            {
                pub fn #original_fn_name (&self, arg : &#backed_type_in )
                {
                    self.proxy.call_noret(method_id!(#fn_name), &arg);
                }
            }

        }
    } else if !args_flag && has_ret
    {
        quote!{

            #func

            impl <'a> #packagename <'a>
            {
                pub fn #original_fn_name (&self) -> #backed_type_ret
                {
                    let out : #backed_type_ret = self.proxy.call_noargs(method_id!(#fn_name));
                    out
                }
            }
        }
    } else
    {
        quote!{

            #func

            impl <'a> #packagename <'a>
            {
                pub fn #original_fn_name (&self)
                {
                    self.proxy.call_noargs_noret(method_id!(#fn_name));
                }
            }
        }
    };

    println!("output : {}", output);

    TokenStream::from(output)
}



