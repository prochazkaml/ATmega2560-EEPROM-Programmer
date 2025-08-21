#!/usr/bin/env python3

# EEPROM/Flash Programmer - eepromgui
#
# A simple GUI based on tkinter to interface with the EERPOM programmer.
#
# pySerial and tkinter (8.6 or newer) need to be installed
#
# 2020 by Stefan Wagner, 2022 Michal Procházka

import os
from eeprom  import Programmer
from tkinter import *
from tkinter import messagebox, filedialog
from tkinter.ttk import *


class Progressbox(Toplevel):
    def __init__(self, root = None, title = 'Please wait!', 
                activity = 'Doing stuff...', value = 0):
        Toplevel.__init__(self, root)
        self.__step = IntVar()
        self.__step.set(value)
        self.__act = StringVar()
        self.__act.set(activity)
        self.title(title)
        self.resizable(width=False, height=False)
        self.transient(root)
        self.grab_set()
        Label(self, textvariable = self.__act).pack(padx = 20, pady = 10)
        Progressbar(self, orient = HORIZONTAL, length = 200, 
                variable = self.__step, mode = 'determinate').pack(
                padx = 10, pady = 10)
        self.update_idletasks()

    def setactivity(self, activity):
        self.__act.set(activity)
        self.update_idletasks()

    def setvalue(self, value):
        self.__step.set(value)
        self.update_idletasks()


def getSelectedSize():
    sizestr = deviceSize.get()
    sizenum = int(sizestr[:-1])
    unit = sizestr[-1]

    if unit == "b":
        return sizenum - 1
    elif unit == "k":
        return sizenum * 1024 - 1
    elif unit == "M":
        return sizenum * 1024 * 1024 - 1
    else:
        return 0


def chipErase():
    eeprom = Programmer()
    if not eeprom.is_open:
        messagebox.showerror('Error', 'Programmer not found!')
        return

    eeprom.sendcommand ('e')

    eeprom.readline()
    eeprom.close()

    messagebox.showinfo('Mission accomplished', 
                'Assuming the chip erase worked.\n\nPlease check its contents to see if it is indeed the case (all bytes should display FF).')
    return


def showContent():
    eeprom = Programmer()
    if not eeprom.is_open:
        messagebox.showerror('Error', 'Programmer not found!')
        return

    startAddr = 0
    endAddr = getSelectedSize()

    progress = Progressbox(mainWindow, 'Please wait!', 'Reading data from device...')

    lines = []

    eeprom.sendcommand ('r', startAddr, endAddr)
    while (startAddr < endAddr):
        bytesline = '%05x:  ' % startAddr
        asciiline = ' '
        i = 16
        while(i):
            data = eeprom.read(1)
            if data:
                bytesline += '%02x ' % data[0]
                if (data[0] > 31) and data[0] < 127: asciiline += chr(data[0])
                else: asciiline += '.'
                i -= 1
        lines.append(bytesline + asciiline)
        startAddr += 16
        progress.setvalue(startAddr * 100 // endAddr)

    progress.destroy()

    contentWindow = Toplevel(mainWindow)
    contentWindow.title('Device memory content')
    contentWindow.minsize(200, 100)
    contentWindow.resizable(width=False, height=True)
    contentWindow.transient(mainWindow)
    contentWindow.grab_set()

    l = Listbox(contentWindow, font = 'TkFixedFont', height = 36, width = 73)
    l.pack(side='left', fill=BOTH)
    s = Scrollbar(contentWindow, orient = VERTICAL, command = l.yview)
    l['yscrollcommand'] = s.set
    s.pack(side='right', fill='y')

    for line in lines:
        l.insert('end', line)

    eeprom.readline()
    eeprom.close()

    contentWindow.mainloop()
    contentWindow.quit()


def uploadBin():
    eeprom = Programmer()
    if not eeprom.is_open:
        messagebox.showerror('Error', 'Programmer not found!')
        return

    maxAddr = getSelectedSize()
    startAddr = 0

    fileName = filedialog.askopenfilename(title = "Select binary file for upload",
                filetypes = (("binary files","*.bin"),("all files","*.*")))
    if not fileName:
        return

    try:
        f = open(fileName, 'rb')
    except:
        messagebox.showerror('Error', 'Could not open file!')
        eeprom.close()
        return

    fileSize = os.stat(fileName).st_size
    if (startAddr + fileSize) > (maxAddr + 1):
        messagebox.showerror('Error', 'Binary file doesn\'t fit into the device!')
        f.close()
        eeprom.close()
        return

    progress = Progressbox(mainWindow, 'Please wait!', 'Writing data to device...')

    dt = deviceType.get()[0]

    eeprom.sendcommand(dt)
    eeprom.readline()

    if dt == "E": eeprom.unlock()

    pagesize = int(devicePage.get().split(" ")[0]) - 1

    datalength = fileSize
    byteswritten = 0
    addr1 = startAddr
    while (datalength):
        count = (addr1 | pagesize) - addr1 + 1
        if count > datalength:
            count = datalength
        addr2 = addr1 + count - 1
        eeprom.sendcommand('p', addr1, addr2)
        eeprom.write(f.read(count))
        byteswritten += count
        progress.setvalue(byteswritten * 100 // fileSize)
        eeprom.readline()
        addr1 += count
        datalength -= count

    if dt == "E": eeprom.lock()

    progress.setactivity('Verifying written data...')
    f.seek(0)
    endAddr = startAddr + byteswritten - 1
    count = byteswritten
    eeprom.sendcommand ('r', startAddr, endAddr)

    while (count):
        data = eeprom.read(1)
        if data:
            if data != f.read(1):
                progress.destroy()
                messagebox.showerror('Error', 'Verification failed!')
                f.close()
                eeprom.close()
                return
            count -= 1
    eeprom.readline()

    f.close()
    eeprom.close()
    progress.destroy()
    messagebox.showinfo('Mission accomplished', 
                str(byteswritten) + ' bytes successfully written!')


def downloadBin():
    eeprom = Programmer()
    if not eeprom.is_open:
        messagebox.showerror('Error', 'Programmer not found!')
        return

    startAddr = 0
    endAddr   = getSelectedSize()
    count     = endAddr - startAddr + 1
    total     = count

    fileName = filedialog.asksaveasfilename(title = "Select output file",
                filetypes = (("binary files","*.bin"),("all files","*.*")))
    if not fileName:
        return

    try:
        f = open(fileName, 'wb')
    except:
        messagebox.showerror('Error', 'Could not open file!')
        eeprom.close()
        return

    progress = Progressbox(mainWindow, 'Please wait!', 'Reading data from device...')

    eeprom.sendcommand ('r', startAddr, endAddr)
    while (count):
        data = eeprom.read(1)
        if data:
            f.write(data)
            count -= 1

        progress.setvalue((total - count) * 100 // total)

    eeprom.readline()
    f.close()
    eeprom.close()
    progress.destroy()
    messagebox.showinfo('Mission accomplished', 'Download completed!')


mainWindow = Tk()
mainWindow.title('EEPROM/Flash Programmer')
mainWindow.resizable(width=False, height=False)

deviceSize = StringVar()
deviceSizes = ["1k", "2k", "4k", "8k", "16k", "32k", "64k", "128k", "256k", "512k", "1M"]
deviceType = StringVar()
deviceTypes = ["EEPROM", "Flash"]
devicePage = StringVar()
devicePages = ["1 byte/page", "2 bytes/page", "4 bytes/page", "8 bytes/page", "16 bytes/page", "32 bytes/page", "64 bytes/page", "128 bytes/page", "256 bytes/page", "512 bytes/page", "1024 bytes/page", "2048 bytes/page", "4096 bytes/page"]

typeFrame = Frame(mainWindow, borderwidth = 2, relief = 'groove')
Label(typeFrame, text = '1. Specify device:').pack(pady = 5)
OptionMenu(typeFrame, deviceSize, deviceSizes[5], *deviceSizes).pack(padx = 10, fill = 'x')
OptionMenu(typeFrame, deviceType, deviceTypes[0], *deviceTypes).pack(padx = 10, fill = 'x', pady = (10, 0))
OptionMenu(typeFrame, devicePage, devicePages[7], *devicePages).pack(padx = 10, fill = 'x', pady = (10, 0))
typeFrame.pack(padx = 10, pady = 10, ipadx = 5, ipady = 5, fill = 'x')

actionFrame = Frame(mainWindow, borderwidth = 2, relief = 'groove')
Label(actionFrame, text = '2. Take your action:').pack(pady = 5)
Button(actionFrame, text = 'Show device content', command = showContent
            ).pack(padx = 10, fill = 'x')
Button(actionFrame, text = 'Binary file to device', command = uploadBin
            ).pack(padx = 10, fill = 'x')
Button(actionFrame, text = 'Device to binary file', command = downloadBin
            ).pack(padx = 10, fill = 'x')
actionFrame.pack(padx =10, pady = (0, 10), ipadx = 5, ipady = 5, fill = 'x')

eraseFrame = Frame(mainWindow, borderwidth = 2, relief = 'groove')
Button(eraseFrame, text = 'PERFORM CHIP ERASE', command = chipErase
            ).pack(padx = 10, pady = 10, fill = 'x')
eraseFrame.pack(padx =10, ipadx = 5, fill = 'x')

Button(mainWindow, text = 'Exit', command = mainWindow.quit).pack(pady = 10)

mainWindow.mainloop()
