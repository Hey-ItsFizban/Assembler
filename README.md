# Assembler
Implementation of an assembler - in C language.

Input: a text file with ".as" suffix, containing text with the Assembly language syntax, let's call it "Haha.as"

Output: 
        "Haha.ob": with a first line containing the number of binary code 'words' and the number of binary data 'words'.
                   After that, each line is a binary 'word',  which is consisted of 12 bits, that is translated to two characters of the                   Base64.
                   
        "Haha.ent": Entry labels file, if needed.
        "Haha.ex":  External labels file, if needed.
        
        
        
        
The program is being tested at the moment, and might not be working properly.
You can contact me at nivuzan90@gmail.com.
