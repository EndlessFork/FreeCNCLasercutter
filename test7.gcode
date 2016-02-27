G21
G28
M649 S100 L10.240 B6; B5=CW-Raster, B6=Pulse-Raster

; raster test 1bpp
M649 C1
G0 X0 Y0
G1 X2.4 Q24 $ghEt
G91
G0 X1
G3 X2.5 Y0.2 R24 Q24 $ghEt
G90

; raster test 2bpp
M649 C2
G0 X0 Y0.2
G1 X2.4 Q24 $wAwDAwzn
G91
G0 X1
G3 X2.5 Y0.2 R24 Q24 $wAwDAwzn
G90

; raster test 3bpp
M649 C3
G0 X0 Y0.4
G1 X2.4 Q24 $4AA4AHAHBTl3
G91
G0 X1
G3 X2.5 Y0.2 R24 Q24 $4AA4AHAHBTl3
G90

; raster test 4bpp
M649 C4
G0 X0 Y0.6
G1 X2.4 Q24 $8AAA8AEjRWeJq83v
G91
G0 X1
G3 X2.5 Y0.2 R24 Q24 $8AAA8AEjRWeJq83v
G90

; raster test 6bpp
M649 C6
G0 X0 Y0.8
G1 X2.4 Q24 $/AAAAA/AAAA/ABCDFHLPXfv_
G91
G0 X1
G3 X2.5 Y0.2 R24 Q24 $/AAAAA/AAAA/ABCDFHLPXfv_
G90

; raster test 8bpp
M649 C8
G0 X0 Y01
G1 X2.4 Q24 $/wAAAAAA/wAAAQIDBQcLDxcfLz9ff7/_
G91
G0 X1
G3 X2.5 Y0.2 R24 Q24 $/wAAAAAA/wAAAQIDBQcLDxcfLz9ff7/_
G90


M649 S10 L1.024 B5; B5=CW-Raster, B6=Pulse-Raster

; raster test 1bpp
M649 C1
G0 X0 Y2
G1 X2.4 Q24 $ghEt
G91
G0 X1
G3 X2.55 Y0.2 R24 Q24 $ghEt
G90

; raster test 2bpp
M649 C2
G0 X0 Y2.2
G1 X2.4 Q24 $wAwDAwzn
G91
G0 X1
G3 X2.5 Y0.2 R24 Q24 $wAwDAwzn
G90

; raster test 3bpp
M649 C3
G0 X0 Y2.4
G1 X2.4 Q24 $4AA4AHAHBTl3
G91
G0 X1
G3 X2.5 Y0.2 R24 Q24 $4AA4AHAHBTl3
G90

; raster test 4bpp
M649 C4
G0 X0 Y2.6
G1 X2.4 Q24 $8AAA8AEjRWeJq83v
G91
G0 X1
G3 X2.5 Y0.2 R24 Q24 $8AAA8AEjRWeJq83v
G90

; raster test 6bpp
M649 C6
G0 X0 Y2.8
G1 X2.4 Q24 $/AAAAA/AAAA/ABCDFHLPXfv_
G91
G0 X1
G3 X2.5 Y0.2 R24 Q24 $/AAAAA/AAAA/ABCDFHLPXfv_
G90

; raster test 8bpp
M649 C8
G0 X0 Y3
G1 X2.4 Q24 $/wAAAAAA/wAAAQIDBQcLDxcfLz9ff7/_
G91
G0 X1
G3 X2.5 Y0.2 R24 Q24 $/wAAAAAA/wAAAQIDBQcLDxcfLz9ff7/_
G90


; raster test 6bpp
M649 S100 L10.24 B6 C6; B5=CW-Raster, B6=Pulse-Raster
G0 X0 Y5
G91
$ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$BACDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789/+
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$CBADEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz012345678/+9
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$DCBAEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz01234567/+98
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$EDCBAFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456/+987
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$FEDCBAGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz012345/+9876
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$GFEDCBAHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz01234/+98765
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$HGFEDCBAIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123/+987654
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$IHGFEDCBAJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz012/+9876543
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$JIHGFEDCBAKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz01/+98765432
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$KJIHGFEDCBALMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0/+987654321
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$LKJIHGFEDCBAMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz/+9876543210
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$MLKJIHGFEDCBANOPQRSTUVWXYZabcdefghijklmnopqrstuvwxy/+9876543210z
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$NMLKJIHGFEDCBAOPQRSTUVWXYZabcdefghijklmnopqrstuvwx/+9876543210zy
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$ONMLKJIHGFEDCBAPQRSTUVWXYZabcdefghijklmnopqrstuvw/+9876543210zyx
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$PONMLKJIHGFEDCBAQRSTUVWXYZabcdefghijklmnopqrstuv/+9876543210zyxw
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$QPONMLKJIHGFEDCBARSTUVWXYZabcdefghijklmnopqrstu/+9876543210zyxwv
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$RQPONMLKJIHGFEDCBASTUVWXYZabcdefghijklmnopqrst/+9876543210zyxwvu
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$SRQPONMLKJIHGFEDCBATUVWXYZabcdefghijklmnopqrs/+9876543210zyxwvut
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$TSRQPONMLKJIHGFEDCBAUVWXYZabcdefghijklmnopqr/+9876543210zyxwvuts
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$UTSRQPONMLKJIHGFEDCBAVWXYZabcdefghijklmnopq/+9876543210zyxwvutsr
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$VUTSRQPONMLKJIHGFEDCBAWXYZabcdefghijklmnop/+9876543210zyxwvutsrq
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$WVUTSRQPONMLKJIHGFEDCBAXYZabcdefghijklmno/+9876543210zyxwvutsrqp
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$XWVUTSRQPONMLKJIHGFEDCBAYZabcdefghijklmn/+9876543210zyxwvutsrqpo
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$YXWVUTSRQPONMLKJIHGFEDCBAZabcdefghijklm/+9876543210zyxwvutsrqpon
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$ZYXWVUTSRQPONMLKJIHGFEDCBAabcdefghijkl/+9876543210zyxwvutsrqponm
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$aZYXWVUTSRQPONMLKJIHGFEDCBAbcdefghijk/+9876543210zyxwvutsrqponml
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$baZYXWVUTSRQPONMLKJIHGFEDCBAcdefghij/+9876543210zyxwvutsrqponmlk
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$cbaZYXWVUTSRQPONMLKJIHGFEDCBAdefghi/+9876543210zyxwvutsrqponmlkj
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$dcbaZYXWVUTSRQPONMLKJIHGFEDCBAefgh/+9876543210zyxwvutsrqponmlkji
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$edcbaZYXWVUTSRQPONMLKJIHGFEDCBAfg/+9876543210zyxwvutsrqponmlkjih
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$fedcbaZYXWVUTSRQPONMLKJIHGFEDCBA/+9876543210zyxwvutsrqponmlkjihg
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$gedcbaZYXWVUTSRQPONMLKJIHGFEDCBA/+9876543210zyxwvutsrqponmlkjihf
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$hgdcbaZYXWVUTSRQPONMLKJIHGFEDCBA/+9876543210zyxwvutsrqponmlkjife
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$ihgcbaZYXWVUTSRQPONMLKJIHGFEDCBA/+9876543210zyxwvutsrqponmlkjfed
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$jihgbaZYXWVUTSRQPONMLKJIHGFEDCBA/+9876543210zyxwvutsrqponmlkfedc
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$kjihgaZYXWVUTSRQPONMLKJIHGFEDCBA/+9876543210zyxwvutsrqponmlfedcb
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$lkjihgZYXWVUTSRQPONMLKJIHGFEDCBA/+9876543210zyxwvutsrqponmfedcba
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$mlkjihgYXWVUTSRQPONMLKJIHGFEDCBA/+9876543210zyxwvutsrqponfedcbaZ
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$nmlkjihgXWVUTSRQPONMLKJIHGFEDCBA/+9876543210zyxwvutsrqpofedcbaZY
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$onmlkjihgWVUTSRQPONMLKJIHGFEDCBA/+9876543210zyxwvutsrqpfedcbaZYX
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$ponmlkjihgVUTSRQPONMLKJIHGFEDCBA/+9876543210zyxwvutsrqfedcbaZYXW
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$qponmlkjihgUTSRQPONMLKJIHGFEDCBA/+9876543210zyxwvutsrfedcbaZYXWV
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$rqponmlkjihgTSRQPONMLKJIHGFEDCBA/+9876543210zyxwvutsfedcbaZYXWVU
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$srqponmlkjihgSRQPONMLKJIHGFEDCBA/+9876543210zyxwvutfedcbaZYXWVUT
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$tsrqponmlkjihgRQPONMLKJIHGFEDCBA/+9876543210zyxwvufedcbaZYXWVUTS
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$utsrqponmlkjihgQPONMLKJIHGFEDCBA/+9876543210zyxwvfedcbaZYXWVUTSR
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$vutsrqponmlkjihgPONMLKJIHGFEDCBA/+9876543210zyxwfedcbaZYXWVUTSRQ
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$wvutsrqponmlkjihgONMLKJIHGFEDCBA/+9876543210zyxfedcbaZYXWVUTSRQP
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$xwvutsrqponmlkjihgNMLKJIHGFEDCBA/+9876543210zyfedcbaZYXWVUTSRQPO
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$yxwvutsrqponmlkjihgMLKJIHGFEDCBA/+9876543210zfedcbaZYXWVUTSRQPON
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$zyxwvutsrqponmlkjihgLKJIHGFEDCBA/+9876543210fedcbaZYXWVUTSRQPONM
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$0zyxwvutsrqponmlkjihgKJIHGFEDCBA/+987654321fedcbaZYXWVUTSRQPONML
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$10zyxwvutsrqponmlkjihgJIHGFEDCBA/+98765432fedcbaZYXWVUTSRQPONMLK
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$210zyxwvutsrqponmlkjihgIHGFEDCBA/+9876543fedcbaZYXWVUTSRQPONMLKJ
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$3210zyxwvutsrqponmlkjihgHGFEDCBA/+987654fedcbaZYXWVUTSRQPONMLKJI
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$43210zyxwvutsrqponmlkjihgGFEDCBA/+98765fedcbaZYXWVUTSRQPONMLKJIH
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$543210zyxwvutsrqponmlkjihgFEDCBA/+9876fedcbaZYXWVUTSRQPONMLKJIHG
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$6543210zyxwvutsrqponmlkjihgEDCBA/+987fedcbaZYXWVUTSRQPONMLKJIHGF
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$76543210zyxwvutsrqponmlkjihgDCBA/+98fedcbaZYXWVUTSRQPONMLKJIHGFE
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$876543210zyxwvutsrqponmlkjihgCBA/+9fedcbaZYXWVUTSRQPONMLKJIHGFED
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$9876543210zyxwvutsrqponmlkjihgBA/+fedcbaZYXWVUTSRQPONMLKJIHGFEDC
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$-9876543210zyxwvutsrqponmlkjihgA/fedcbaZYXWVUTSRQPONMLKJIHGFEDCB
G1 X12.8 Q64 
G0 X-12.8 Y0.2
$_-9876543210zyxwvutsrqponmlkjihgfedcbaZYXWVUTSRQPONMLKJIHGFEDCBA
G1 X12.8 Q64 
G0 X-12.8 Y0.2


(footer)
G90
G0 X0 Y230
M0
M2
