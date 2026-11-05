---
layout: default
lang: ja
title: numeric 0.2.0 使い方ガイド
description: EPICSのnumericレコードでINPA、ARGA、計算結果を使うための日本語ガイド
permalink: /ja/
---

<p class="eyebrow">numeric 0.2.0 guide</p>

# 入力から計算結果まで

<p class="lead">numericのレコードに、どのPVを入力として渡し、どの引数を設定し、どう結果を読むかを説明します。例は同梱のIOCで試せます。</p>

<div class="callout" markdown="1">
このガイドはリリース準備中の0.2.0向けです。公開済みの0.1系とは利用できる操作やフィールドが異なります。[インストール手順](https://github.com/akared819/numeric#build-and-test)に従って対象のソースをビルドしてください。numericの実行にPythonは不要です。
</div>

[フィールドの役割](#fields) · [入力リンク](#inputs) · [計算の引数](#arguments) · [実際に試す](#recipes) · [操作一覧](#operations) · [困ったとき](#troubleshooting)

<h2 id="first-steps">まずは平均を計算する</h2>

`numericScalar`は、計算結果が**一つの数値**になる操作に使います。入力が配列でも構いません。例のIOCを起動し、別の端末でEPICS Baseの`caput`と`caget`を使ってください。

```sh
# リポジトリのルートで。Baseのパスは自分の環境に変更します。
./bin/$(/absolute/path/to/base/startup/EpicsHostArch)/numericExample examples/ioc/st.cmd
```

```sh
# 別の端末。最初の「4」は配列の要素数です。
caput -a demo:scalarInput 4 1 2 3 4
caput demo:scalarMean.PROC 1
caget demo:scalarMean.VAL          # 2.5
caget demo:scalarMean.SEVR         # NO_ALARM を確認
```

`.PROC`への書き込みで計算を実行します。入力PVを書き換えただけでは、通常この計算レコードは処理されません。結果が配列になる操作には`numericArray`を使い、`caget -a レコード名.VAL`で取得します。

NumPyに慣れている方は、上の例を`mean([1, 2, 3, 4])`のような計算と考えられます。ただしnumericはPythonを呼び出すものではなく、NumPyの全機能や既定値を再現するものでもありません。例えば`stddev`は標本標準偏差（分母N−1）、`population_stddev`は母標準偏差（分母N）です。NumPyの配列と引数の考え方は[NumPy入門](https://numpy.org/doc/stable/user/absolute_beginners.html)も参照できます。

<h2 id="fields">フィールドは何を表す？</h2>

| フィールド | 役割 | 例 |
| --- | --- | --- |
| `OP` | 実行する操作名。レコードを読み込むときに設定 | `mean`, `correlation`, `gaussian_smooth` |
| `INP` | 入力が一つのときに使える従来の入力リンク | `"SENSOR:WAVE NPP NMS"` |
| `INPA`～`INPP` | 入力PVへのリンク。操作が複数入力を取る場合、Aから順番に設定 | `INPA`にX、`INPB`にY |
| `ARGA`～`ARGP` | 操作の**設定引数**を指定する文字列フィールド | `ARGA`に窓幅`"5"` |
| `NMAX` | 入力波形の最大要素数。`OP`と同様、実行中には変更不可 | `"4096"` |
| `OMAX` | `numericArray`の出力を入れる最大要素数。実行中には変更不可 | FFTの出力には入力の2倍が必要な場合あり |
| `VAL` | 計算結果。`numericScalar`は数値、`numericArray`は配列 | `caget record.VAL` / `caget -a record.VAL` |
| `NORD`, `NIN` | 要素数。`numericArray`の`NORD`は出力数、`NIN`は最初の入力数 | 容量ではなく実際に扱った個数 |
| `PROC`, `STAT`, `SEVR` | 処理を起動、警報の種類と重大度を確認 | `.PROC 1`、`.SEVR` |

`numericScalar.NORD`は**出力数ではなく最初の入力の要素数**です。失敗時、`numericScalar`は前回の`VAL`を保持して`NORD`を0にし、`numericArray`は前回の`VAL`と`NORD`を保持して`NIN`を0にします。どちらも警報を確認してください。

<h2 id="inputs">INPAとINPB：どのデータを渡す？</h2>

`INPA`などには数値そのものではなく、**値を提供するPVへのリンク**を設定します。波形も一つの数値もリンクできます。何をA、B、Cに入れるかは`OP`ごとに決まります。例えば、相関係数は二つの同じ長さの波形を使います。

```db
record(numericScalar, "SIGNAL:CORR") {
    field(OP,   "correlation")
    field(INPA, "SIGNAL:X NPP NMS")  # 1番目の波形
    field(INPB, "SIGNAL:Y NPP NMS")  # 2番目の波形
    field(NMAX, "4096")
}
```

このコードを自分のIOCに追加する場合は、`SIGNAL:X`と`SIGNAL:Y`を実際の波形PVに置き換えてください。`NPP`はリンク先をここで受動処理しない設定、`NMS`はリンク先の重大度をこのリンクから引き継がない設定です。リンク先の警報を結果側に自動で反映したい場合は、EPICSのリンク設定を別途検討してください。入力リンクは順番に読み取られ、複数PVが同時刻のスナップショットになる保証はありません。

Channel Accessリンクには、更新時の処理を要求する`CP`／`CPP`もあります。リンクの種類によって処理の意味が異なるため、使う場合は処理回数やデータの組合せを検証してください。詳しくは[EPICSのリンク処理の説明](https://docs.epics-controls.org/en/latest/appdevguide/lockScanProcess.html)を参照してください。

一入力の操作には`INP`または`INPA`を使えます。**同じレコードで`INP`と`INPA`などを併用しないでください。** 複数入力ではAから順に必要なリンクを設定し、省略や余分なリンクを置かないでください。入力リンクには定数ではなくPVを指定します。

専用レコードは`DOUBLE`、`FLOAT`、符号付き`LONG`、ローカルDBリンクの`INT64`を受け付けます。`INT64`はDOUBLEに正確に変換できない値を拒否します。Channel Access経由で送信元がDOUBLEとして公開した値は、到着前に丸められていても判別できません。[入力型の詳細](https://github.com/akared819/numeric/blob/main/docs/numeric-scalar.md)を確認してください。

<h2 id="arguments">ARGAとARGB：計算の設定を渡す</h2>

`ARGA`～`ARGP`は、波形データとは別の**設定値**です。文字列として書き込み、操作に応じて整数や小数として解釈されます。例えば`gaussian_smooth`では`ARGA`が窓幅、`ARGB`が平滑化の`alpha`です。

| `OP` | `ARGA` | `ARGB` | `ARGC` | 空欄の扱い |
| --- | --- | --- | --- | --- |
| `moving_mean`, `moving_median` | 窓幅：3～4095の奇数 | ― | ― | 従来の`WIN`を使用 |
| `gaussian_smooth` | 窓幅：3～4095の奇数 | `alpha`：0.01～10 | ― | それぞれ`WIN`、`ALPH`を使用 |
| `histogram_uniform` | 下限 | 上限 | ビン数：1～65536の整数 | `HMIN`、`HMAX`、`NBIN`を使用 |
| `array_scale` | 倍率：有限の小数 | ― | ― | 既定値1を使用 |
| `pow_int` | 指数：−1024～1024の整数 | ― | ― | 既定値2を使用 |

非空の`ARG`は対応する従来フィールドより優先されます。空文字に戻すと、従来フィールド、または従来フィールドのない引数の既定値が使われます。操作にない`ARG`を設定するとエラーです。文字列は空白なし・38文字以内で指定します。引数の変更だけでは計算されず、**次の処理時**に反映されます。複数のフィールドを別々に書き込む場合、途中の組合せで処理されないよう調整してください。

窓幅は入力波形の要素数以下にしてください。Gaussianの`alpha`は窓内の重みの広がりを決めます。窓幅をKとすると標準偏差は`(K−1)/(2*alpha)`サンプルです。従来フィールド`ALPH`の初期値は約1.17741です。[平滑化の詳細](https://github.com/akared819/numeric/blob/main/docs/window-statistics.md)も参照してください。

> 注意：数値だからといって常に`ARG`とは限りません。`welch_psd`の区間長やサンプル間隔は設定用の**入力PV**なので`INPB`～`INPD`で渡します。[操作一覧](#operations)で確認してください。

<h2 id="recipes">同梱IOCで試す</h2>

### 1. 二つの波形の相関

```sh
caput -a demo:multiX 4 1 2 3 4
caput -a demo:multiY 4 2 4 6 8
caput demo:multiCorrelation.PROC 1
caget demo:multiCorrelation.VAL      # 約 1
caget demo:multiCorrelation.SEVR
```

`demo:multiCorrelation`の`INPA`は`multiX`、`INPB`は`multiY`です。両方の書き込み後に処理します。

### 2. 引数を変更して移動平均

```sh
caput -a demo:arrayInput 5 1 2 3 4 5
caput demo:arrayMean.ARGA 5
caput demo:arrayMean.PROC 1
caget -a demo:arrayMean.VAL          # 約 1.6 2.2 3 3.8 4.4
caget demo:arrayMean.SEVR
```

窓幅5の中心移動平均です。端では端の値を繰り返して補います。`ARGA`を空文字に戻すと、例のDBの`WIN=3`が再び使われます。偶数などの無効な窓幅では`INVALID`になり、前の出力が残ります。

### 3. スカラーの引数：整数乗

```sh
caput demo:multiPoint 2
caput demo:multiPower.ARGA 3
caput demo:multiPower.PROC 1
caget demo:multiPower.VAL            # 8
```

値2は入力リンク`INP`から、指数3は`ARGA`から渡ります。`ARGA`が空なら指数は2です。

### 4. Welch法によるPSD

```sh
caput -a demo:SPECTRAL:INPUT 16 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1
caput demo:SPECTRAL:PSD.PROC 1
caget -a demo:SPECTRAL:PSD.VAL
caget demo:SPECTRAL:PSD.SEVR
```

例では、`INPA`が波形、`INPB`が区間長8、`INPC`が重なり4、`INPD`がサンプル間隔0.01秒のPVです。これらは`ARGA`ではありません。出力は0 Hzからナイキスト周波数までの片側PSDで、5要素です。[スペクトル解析の規約](https://github.com/akared819/numeric/blob/main/docs/fft.md)も確認してください。

<h2 id="operations">操作別：どの入力と引数を使う？</h2>

次の表で`配列`は波形PV、`値`は一つの数値を持つPVです。`ARG`欄が「なし」の操作には`ARGA`などを設定しません。入力が一つなら`INPA`の代わりに`INP`も使えます。

### 数値を一つ返す：numericScalar

| `OP` | 入力リンク（順番） | 設定引数 | 結果・条件 |
| --- | --- | --- | --- |
| `mean` | A: 配列 | なし | 平均。1要素以上 |
| `variance`, `stddev` | A: 配列 | なし | 標本分散／標本標準偏差。2要素以上、分母N−1 |
| `population_variance`, `population_stddev` | A: 配列 | なし | 母分散／母標準偏差。分母N |
| `covariance`, `correlation` | A: X配列、B: Y配列 | なし | 同じ長さで2要素以上。相関では定数配列不可 |
| `weighted_mean` | A: データ配列、B: 重み配列 | なし | 同長。重みは非負で一つ以上が正 |
| `quantile_sorted` | A: 昇順配列、B: 割合の値 | なし | Bは0～1。昇順でない入力は拒否 |
| `vector_norm` | A: 配列 | なし | ユークリッドノルム |
| `erf`, `gamma`, `log1p`, `normal_cdf` | A: 値 | なし | 各関数の定義域を確認 |
| `pow_int` | A: 底の値 | `ARGA`: 整数指数、既定2 | 底の指数乗 |
| `polynomial_eval` | A: 係数配列、B: 評価点 | なし | 係数は定数項から昇べき順 |
| `interp_linear`, `interp_cspline` | A: X配列、B: Y配列、C: 評価点 | なし | Xは狭義単調増加、評価点は範囲内 |
| `polynomial_integral`, `polynomial_root` | A: 係数配列、B: 下限、C: 上限 | なし | 積分または区間内の根。根には有効な括り区間が必要 |
| `matrix_determinant` | A: 行優先の行列配列、B: 次元の値 | なし | Aの要素数は次元×次元 |

### 配列を返す：numericArray

| `OP` | 入力リンク（順番） | 設定引数 | 出力 |
| --- | --- | --- | --- |
| `array_scale` | A: 配列 | `ARGA`: 倍率、既定1 | 各要素を倍率で掛けた配列 |
| `moving_mean`, `moving_median` | A: 配列 | `ARGA`: 窓幅 | 入力と同じ長さ |
| `gaussian_smooth` | A: 配列 | `ARGA`: 窓幅、`ARGB`: alpha | 入力と同じ長さ |
| `histogram_uniform` | A: 配列 | `ARGA`: 下限、`ARGB`: 上限、`ARGC`: ビン数 | ビンごとの個数。範囲は下限を含み上限を含まない |
| `linear_fit` | A: X配列、B: Y配列 | なし | 切片、傾き、共分散3値、残差平方和の6要素 |
| `fft_real_radix2_forward` | A: 実数配列 | なし | N個の複素数を実部・虚部交互の2N要素で出力。Nは2の累乗 |
| `fft_real_radix2_inverse` | A: 共役対称な複素配列 | なし | N個の実数。逆変換は1/Nで正規化 |
| `fft_complex_radix2_forward`, `fft_complex_radix2_inverse` | A: 複素配列 | なし | 実部・虚部交互。複素数の個数は2の累乗 |
| `fft_complex_mixed_forward`, `fft_complex_mixed_inverse` | A: 複素配列 | なし | 正の任意の複素数個数。逆変換は1/Nで正規化 |
| `fft_frequency_axis` | A: サンプル数、B: サンプル間隔 | なし | 周波数軸（Hz）。偶数長のナイキスト成分は負側 |
| `window_hann`, `spectrum_power` | A: 波形／複素スペクトル | なし | Hann窓適用／正規化なしの振幅二乗 |
| `welch_psd` | A: 波形、B: 区間長、C: 重なり、D: サンプル間隔 | なし | 片側PSD。区間長は4以上の2の累乗 |
| `stft_hann` | A: 波形、B: フレーム長、C: ホップ幅 | なし | フレーム順に複素数を並べた配列 |
| `matrix_eigenvalues_symmetric` | A: 行優先の対称行列、B: 次元 | なし | 実固有値を昇順で出力 |

`OMAX`は**出力のDOUBLE要素数**です。例えば実数FFTの入力がN要素なら、複素スペクトルは2N要素なので`OMAX >= 2N`が必要です。STFTはフレーム数×2Nが必要です。詳細な制約・正規化・メモリ使用量は[配列レコード](https://github.com/akared819/numeric/blob/main/docs/numeric-array.md)、[FFT](https://github.com/akared819/numeric/blob/main/docs/fft.md)、[GSL対応表](https://github.com/akared819/numeric/blob/main/docs/gsl-coverage.md)を参照してください。

これらは専用レコードから利用できる操作です。状態を持つストリーミング統計や、C++の関数を渡すソルバーは異なるインターフェースを使います。Pythonの関数や生の関数ポインターを`ARG`フィールドに設定することはできません。[ストリーミング](https://github.com/akared819/numeric/blob/main/docs/streaming.md)、[ネイティブAPI](https://github.com/akared819/numeric/blob/main/docs/numerical-differentiation.md)も参照してください。

<h2 id="troubleshooting">困ったとき</h2>

1. 値が変わらないときは、まず正しいレコードの`.PROC`を処理したか確認します。
2. `.SEVR`が`INVALID`なら、入力リンク、要素数、`NMAX`／`OMAX`、`OP`に対応する引数を確認します。**古い結果が残ることがあるため、値だけを新しい結果として使わないでください。**
3. 単一入力の`INP`と`INPA`の併用、複数入力のリンク抜け、未使用の`ARG`への書き込みがないか確認します。
4. 同じIOCで他のGSL利用モジュールと共存する場合は、[GSLエラー処理の初期化](https://github.com/akared819/numeric/blob/main/docs/architecture.md#gsl-setup-for-an-ioc)を確認します。

EPICS Baseの通知に関する断続的なクラッシュは未解決です。また、aSubでは入力取得の失敗がnumericの処理前に起き、期待した警報にならない場合があります。運用するIOCと環境で十分に試験し、必要に応じて入力の鮮度を別途監視してください。詳しくは[0.2.0開発ノート](https://github.com/akared819/numeric/blob/main/docs/release-0.2.0-draft.md)を参照してください。
