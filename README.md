# pdl
The module is designed as a common library for processing binary protocol, for example, the following is a bit stream which will be processed:
  00000000 00000000 00000000 00000000 00000000 00000000 00000000 00110010 00000000 00000000 00000000 01100100 00000000 00000000 00000000 01100100 00000000 00000000 00000000 00010000
Obviously, it is very confused for understanding, but if we convert above bit stream to the following xml format:
  <bm_type size=32 pos=0 value=0/> <bm_width size=32 pos=32 value=50/> <bm_height size=32 pos=64 value=100/> <bm_width_bytes size=32 pos=96 value=100/> <bm_planes size=16 pos=128 value=0/> <bm_bits_pixel size=16 pos=144 value=16/>
You may immediately figure out it is a BITMAP header.

Actually, in most cases, a binary protocol is very complicated, such as video/audio stream, so I designed the module to help myself to process those kinds of binary protocols easier, I hope the module is useful for you, too.

To use this module, you need to know how to descript a binary protocol. At first, you need to know how to descript a field of the protocol, as a binary protocol is always composed with many fields, and all fields are organized as a tree. Here is a demo of binary protocol:
  struct demo_protocol {
    uint32_t _type;
    struct {
      uint8_t _flag1 : 1;
      uint8_t _flag2 : 1;
      uint8_t _flag3 : 2;
      uint8_t _flag4 : 4;
    } _flags;
    union {
      struct {
        // ...
      } _a;
      struct {
        // ...
      } _b;
    } _adapt; // the actual data type is indicated by _flags field.
    uint32_t _size;
    uint8_t _data{1}; // the actual size is indicated by _size field.
  };

There are two kinds of fields in above demo: leaf-field & combined-field. For leaf-field, we can descript it with 3 properties: 'name', 'size' (in bit) & 'amount' -- you can add a 'type' property if you like, but 3 properties are enough. For combined-field, as similar, we can descript it with 'name', 'total-size' (in bit) & 'amount' properties. And then, we can descript the ownership with a "block-diagram" which looks like:
  [demo_protocol [_type:32][_flags [_flag1:1][_flag2:1][_flag3:2][_flag4:4]]{_adapt_a ...}{_adapt_b ...}[_size:32]{_data:8}]
  Note: [] meams essential field ('amount' property is 1); {} means optional field ('amount' property depends on other fields); the 'size' property is divided with the 'name' property by ':' symbol -- it is omitted if the field is a combined-field.
The above diagram didn't tell us the dependency relationship of the optional fields, we can descript it by a pair list:
  (_flags,_adapt_a) (_flags,_adapt_b) (_size,_data)
  Note: the first item of each pair is the dependent field of the second item, which means we can calculate the amount of the second item by the value of the dependent field, the formula is AmountOf( ValueOf('dep-field bits') ).

To understand above diagram, please think about a bit stream as the following "block-diagram":
  [ [00000000 00000000 00000000 00000000][ [1][1][01][1010] ]{...}[00000000 00000000 00000000 00000100]{00000001 00000010 00000011 00000100} ]
Please compare with the "block-diagram" of above protocol description, you should understand the protocol description just descripted the format of above bit stream, and we can figure out what the meaning of the '{...}' block is by the dependency relationship list.

Please make sure you have understood what I said in the above. The thinking of protocol description can not only process the fixed length fields, but also the unfixed length fields, the way is using a function to calculate the size (in bit) by the field's bits, instead of the constant.

Here is a roadmap about the module:
  1.0 (DONE) - provided a group of C++ interfaces/classes to descript a binary protocal, you need to derive field descriptors from 'leaf_field_des' or 'combined_field_des' interface/class, and organize your field descriptors as 'field_des_tree' & 'field_des_dependency', and then derive a callback from 'combined_field_des::parse_callback' to handle the 'field_info' items. Please refer the FIELD_DES_UT as an example where in the field_des.cpp file.
  1.1 (DOING) - support converting protocol between binary and/or text format, which means users don't have to implement the 'combined_field_des::parse_callback'.
  1.2 (FUTURE) - support descripting a binary protocal by (text format) Protocol Description Language, which means users can use the module more friendly.
  1.3 (FUTURE) - porting the module in other languages or frameworks.

Please contact me ching_li@163.com if you have any quesions, and I'm very glad if you like to join the project to help make the module better.
