; honyaos-ipl
; TAB=4

CYLS	EQU		10				; どこまで読み込むか

		ORG		0x7c00			; 0x7c00にプログラムを読み込む

; 以下は標準的なFAT12フォーマットフロッピーディスクのための記述

		JMP		entry
		DB		0x90
		DB		"HONYAIPL"		; ブートセクタの名前を自由に書いてよい（8バイト）
		DW		512				; 1セクタの大きさ（512にしなければいけない）
		DB		1				; クラスタの大きさ（1セクタにしなければいけない）
		DW		1				; FATがどこから始まるか（普通は1セクタ目からにする）
		DB		2				; FATの個数（2にしなければいけない）
		DW		224				; ルートディレクトリ領域の大きさ（普通は224エントリにする）
		DW		2880			; このドライブの大きさ（2880セクタにしなければいけない）
		DB		0xf0			; メディアのタイプ（0xf0にしなければいけない）
		DW		9				; FAT領域の長さ（9セクタにしなければいけない）
		DW		18				; 1トラックにいくつのセクタがあるか（18にしなければいけない）
		DW		2				; ヘッドの数（2にしなければいけない）
		DD		0				; パーティションを使ってないのでここは必ず0
		DD		2880			; このドライブ大きさをもう一度書く
		DB		0,0,0x29		; よくわからないけどこの値にしておくといいらしい
		DD		0xffffffff		; たぶんボリュームシリアル番号
		DB		"HONYA-OS   "	; ディスクの名前（11バイト）
		DB		"FAT12   "		; フォーマットの名前（8バイト）
		RESB	18				; とりあえず18バイトあけておく

; プログラム本体

entry:
		MOV		AX, 0			; レジスタ初期化
		MOV		SS, AX
		MOV		SP, 0x7c00
		MOV		DS, AX

; ディスクを読む

		MOV		AX, 0x0820
		MOV		ES, AX
		MOV		CH, 0			; シリンダ0
		MOV		DH, 0			; ヘッド0
		MOV		CL, 2			; セクタ2

readloop:
		MOV		SI, 0			; 失敗回数を数えるレジスタ

retry:
		MOV		AH, 0x02		; AH=0x02 : ディスク読み込み
		MOV		AL, 1			; 1セクタ
		MOV		BX, 0
		MOV		DL, 0x00		; Aドライブ
		INT		0x13			; ディスクBIOS呼び出し
		JNC		next			; エラーが起きなければnextへ
		ADD		SI, 1			; SIに1を足す
		CMP		SI, 5			; SIを5と比較
		JAE		error			; SI >= 5 だったらerrorへ
		MOV		AH, 0x00
		MOV		DL, 0x00		; Aドライブ
		INT		0x13			; ドライブのリセット
		JMP		retry

next:
		MOV		AX, ES			; アドレスを0x200進める
		ADD		AX, 0x20
		MOV		ES, AX			; ADD ES, 0x20 という命令が無いのでこうしている
		ADD		CL, 1			; CL（セクタ）に1加算
		CMP		CL, 18			; CL（セクタ）と18を比較
		JBE		readloop		; CL <= 18 だったらreadloopへ
		MOV		CL, 1			; CL（セクタ）を1に初期化
		ADD		DH, 1			; DH（ヘッド）に1加算
		CMP		DH, 2			; DH（ヘッド）と2を比較
		JB		readloop		; DH < 2 だったらreadloopへ
		MOV		DH, 0			; DH（ヘッド）を0に初期化
		ADD		CH, 1			; CH（シリンダ）に1加算
		CMP		CH, CYLS		; CH（シリンダ）とCYLSを比較
		JB		readloop		; CH < CYLS だったらreadloopへ

; 読み終わったのでharibote.sysを実行
		MOV		[0x0ff0], CH	; IPLがどこまで読んだのかをメモ
		JMP		0xc200

error:
		MOV		AX, 0
		MOV		ES, AX
		MOV		SI, msg

putloop:
		MOV		AL, [SI]
		ADD		SI, 1			; SIに1を足す
		CMP		AL, 0
		JE		fin
		MOV		AH, 0x0e		; 一文字表示ファンクション
		MOV		BX, 15			; カラーコード
		INT		0x10			; ビデオBIOS呼び出し
		JMP		putloop

fin:
		HLT						; 何かあるまでCPUを停止
		JMP		fin				; 無限ループ

msg:
		DB		0x0a, 0x0a		; 改行を2つ
		DB		"load error"
		DB		0x0a			; 改行
		DB		0

		RESB	0x7dfe-$		; 0x001feまでを0x00で埋める命令

		DB		0x55, 0xaa
