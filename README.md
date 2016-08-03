# pdl

The module is designed as a common library for processing binary protocol, for example, here is a bit stream which would be processed: <br/>
&nbsp;&nbsp;&nbsp;&nbsp;00000000 00000000 00000000 00000000 00000000 00000000 00000000 00110010 00000000 00000000 00000000 01100100 00000000 00000000 00000000 01100100 00000000 00000000 00000000 00010000 <br/>
Obviously, it is very confused for understanding, but if we convert above bit stream to the following xml format: <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&lt;bm_type size=32 pos=0 value=0/&gt; &lt;bm_width size=32 pos=32 value=50/&gt; &lt;bm_height size=32 pos=64 value=100/&gt; &lt;bm_width_bytes size=32 pos=96 value=100/&gt; &lt;bm_planes size=16 pos=128 value=0/&gt; &lt;bm_bits_pixel size=16 pos=144 value=16/&gt; <br/>
You may immediately figure out it is a BITMAP header. <br/>
<br/>
Actually, in most cases, a binary protocol is very complicated, such as video/audio stream, so I designed the module to help myself to process those kinds of binary protocols easier, I hope the module is useful for you, too.<br/>
<br/>
To use this module, you need to know how to descript a binary protocol. At first, you need to know how to descript a field of the protocol, as a binary protocol is always composed with many fields, and all fields are organized as a tree. Here is a demo of binary protocol: <br/>
&nbsp;&nbsp;&nbsp;&nbsp;struct demo_protocol {                                                                      <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;uint32_t _type;                                                     <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct {                                                            <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;uint8_t _flag1 : 1;                         <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;uint8_t _flag2 : 1;                         <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;uint8_t _flag3 : 2;                         <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;uint8_t _flag4 : 4;                         <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;} _flags;                                                           <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;union {                                                             <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct {                                    <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;// ...              <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;} _a;                                       <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;struct {                                    <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;// ...              <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;} _b;                                       <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;} _adapt; // the actual data type is indicated by _flags field.     <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;uint32_t _size;                                                     <br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;uint8_t _data[1]; // the actual size is indicated by _size field.   <br/>
&nbsp;&nbsp;&nbsp;&nbsp;};                                                                                          <br/>
There are two kinds of fields: leaf-field &amp; combined-field. For leaf-field, we can descript it with 3 properties: 'name', 'size' (in bit) &amp; 'count' -- you can add a 'type' property if you like, but 3 properties are enough for discussing the thinking. For combined-field, as similar, we can descript it with 'name', 'total-size' (in bit) &amp; 'count' properties. And then, we can descript the ownership with a &quot;block-diagram&quot; which looks like: <br/>
&nbsp;&nbsp;&nbsp;&nbsp;[demo_protocol [_type:32][_flags [_flag1:1][_flag2:1][_flag3:2][_flag4:4]]{_adapt_a ...}{_adapt_b ...}[_size:32]{_data:8}] <br/>
&nbsp;&nbsp;&nbsp;&nbsp;Note: [] meams essential field ('count' property is 1); {} means optional field ('count' property depends on other fields); the 'size' property is divided with the 'name' property by ':' symbol -- it is omitted if the field is a combined-field. <br/>
Above diagram didn't tell us the dependency relationship of the optional fields, we can descript them by a pair list: <br/>
&nbsp;&nbsp;&nbsp;&nbsp;(_flags,_adapt_a) (_flags,_adapt_b) (_size,_data) <br/>
&nbsp;&nbsp;&nbsp;&nbsp;Note: the first item of each pair is the dependent field of the second item, which means we can calculate the count of the second item by the value of the dependent field, the formula is CountOf( ValueOf('dep-field bits') ). <br/>
To understand above diagram, please think about a bit stream as the following &quot;block-diagram&quot;: <br/>
&nbsp;&nbsp;&nbsp;&nbsp;[ [00000000 00000000 00000000 00000000][ [1][1][01][1010] ]{...}[00000000 00000000 00000000 00000100]{00000001 00000010 00000011 00000100} ] <br/>
Please compare with the &quot;block-diagram&quot; of above protocol description, you can see the protocol description just descripted the format of above bit stream, and by the dependency relationship list, we can figure out what the meaning of the '{...}' block is. <br/>
<br/>
Please make sure you have understood what I said in the above. The thinking of protocol description can not only process the fixed length fields, but also the unfixed length fields, the way is that define a function to calculate the size (in bit) by the field's bits, instead of the constant. <br/>
<br/>
Here is a roadmap of the module: <br/>
&nbsp;&nbsp;&nbsp;&nbsp;1.0 (DONE) - provided a group of C++ interfaces/classes to descript a binary protocal, users need to derive field descriptors from 'leaf_field_des' or 'combined_field_des' interface/class, and organize the field descriptors as 'field_des_tree' &amp; 'field_des_dependency', and then derive a callback from 'combined_field_des::parse_callback' to handle the 'field_info' items. Please refer the FIELD_DES_UT as an example where in the field_des.cpp file. <br/>
&nbsp;&nbsp;&nbsp;&nbsp;1.1 (DOING) - support converting protocol between binary and text format, which means users don't have to implement the 'combined_field_des::parse_callback'. <br/>
&nbsp;&nbsp;&nbsp;&nbsp;1.2 (FUTURE) - support descripting a binary protocal by (text format) Protocol Description Language, which means the module is more friendly to use. <br/>
&nbsp;&nbsp;&nbsp;&nbsp;1.3 (FUTURE) - porting the module in other languages or frameworks. <br/>
<br/>
Please contact me ching_li@163.com if you have any quesions, and I'm very glad if you like to join the project to help make the module better. <br/>
