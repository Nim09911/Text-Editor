# DSA
***
***Data Structures and Algorithms (mini project)***\
**Text editor**

JUMPING DIRECT TO 11
  Lots of changes...horizontal scrolling, fixed backspace, added read mode, can open and save unsaved(not made before) and empty(seg fault before) files
  Working on Resizing now
UPDATE(5&6):\
ADDED: Vertical (page scrolling) by reprinting lines according to offset/page_number\
OLD(4):\
FIXED: newline error on saving which was the root cause of issues esp for backspace and enter has been fixed\
OLD(3):\
Simple backspace rendering has now been fixed\
*Still issues with saving in regards to backspace and enter cases when shifting current buffer text to the next*\
OLD(1&2):\
Enter enters a newline at the next position regardless of cursor position\
Backspace deletes the entire line if at 0 poition regardless of buffer length/size\
*(working on)\
Enter shifts ahead text to next newline but the deletion of the ahead text in the buffer is causing some issues*\
