#!/usr/bin/python
# -*- coding: utf-8 -*-

import re
import sys

# from https://twitter.com/ICFPContest2015/status/629422866149175296
tweet = u"אין זיין הויז בייַ ר'ליעה טויט קטהולהו ווייץ דרימינג."

# from Wikipedia Yidish
table = u"""<table class="wikitable sortable" rules="all" align="center" style="text-align: center; border: 1px solid darkgray;" cellpadding="3">
<tr>
<th>Symbol</th>
<th>YIVO Romanization</th>
<th>Harkavy Romanization</th>
<th>IPA Transcription</th>
<th>LCAAJ Transcription</th>
<th>Name</th>
<th>Notes</th>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">א</span></span></td>
<td colspan="4">(none)</td>
<td><i>shtumer <a href="/wiki/Aleph_(letter)" title="Aleph (letter)" class="mw-redirect">alef</a></i></td>
<td>Indicates that a syllable starts with the vocalic form of the following letter. Neither pronounced nor transcribed.</td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">אַ</span></span></td>
<td colspan="4"><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">a</span></td>
<td><i>pasekh alef</i></td>
<td>As a non-YIVO equivalent, an [a] may also be indicated by an unpointed <i>alef</i>.</td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">אָ</span></span></td>
<td colspan="2">o</td>
<td><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">ɔ</span></td>
<td>o</td>
<td><i>komets alef</i></td>
<td>As a non-YIVO equivalent, an [o] may also be indicated by an unpointed <i>alef</i>.</td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ב</span></span></td>
<td colspan="4"><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">b</span></td>
<td><i><a href="/wiki/Beth_(letter)" title="Beth (letter)" class="mw-redirect">beys</a></i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">בּ</span></span></td>
<td colspan="2">(none)</td>
<td><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">b</span></td>
<td>(b)</td>
<td><i>beys</i></td>
<td>Non-YIVO alternative to ב.</td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">בֿ</span></span></td>
<td colspan="4"><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">v</span></td>
<td><i>veys</i></td>
<td>Used only in words of Semitic origin.</td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ג</span></span></td>
<td colspan="4"><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">ɡ</span></td>
<td><i><a href="/wiki/Gimel_(letter)" title="Gimel (letter)" class="mw-redirect">giml</a></i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ד</span></span></td>
<td colspan="4"><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">d</span></td>
<td><i><a href="/wiki/Daleth_(letter)" title="Daleth (letter)" class="mw-redirect">daled</a></i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">דזש</span></span></td>
<td>dzh</td>
<td>(none)</td>
<td><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">d͡ʒ</span></td>
<td>dž</td>
<td><i>daled zayen shin</i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ה</span></span></td>
<td colspan="4"><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">h</span></td>
<td><i><a href="/wiki/He_(letter)" title="He (letter)">hey</a></i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ו</span></span></td>
<td colspan="2">u</td>
<td><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">ʊ</span></td>
<td>u</td>
<td><i><a href="/wiki/Waw_(letter)" title="Waw (letter)">vov</a></i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">וּ</span></span></td>
<td>u</td>
<td>(none)</td>
<td><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">ʊ</span></td>
<td>u</td>
<td><i>melupm vov</i></td>
<td>Used only adjacent to ו or before י.</td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">וֹ</span></span></td>
<td colspan="2">(none)</td>
<td><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">ɔ</span>, <span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">ɔj</span></td>
<td>(o,oj)</td>
<td><i>khoylem</i></td>
<td>Non-YIVO alternative to אָ and וי.</td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">װ</span></span></td>
<td colspan="4">v</td>
<td><i>tsvey vovn</i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">וי</span></span></td>
<td>oy</td>
<td>oi</td>
<td><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">ɔj</span></td>
<td>oj</td>
<td><i>vov yud</i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ז</span></span></td>
<td colspan="4"><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">z</span></td>
<td><i><a href="/wiki/Zayin" title="Zayin">zayen</a></i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">זש</span></span></td>
<td colspan="2">zh</td>
<td><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">ʒ</span></td>
<td>ž</td>
<td><i>zayen shin</i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ח</span></span></td>
<td>kh</td>
<td>ch</td>
<td colspan="2"><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">x</span></td>
<td><i><a href="/wiki/Heth_(letter)" title="Heth (letter)" class="mw-redirect">khes</a></i></td>
<td>Used only in words of Semitic origin.</td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ט</span></span></td>
<td colspan="4"><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">t</span></td>
<td><i><a href="/wiki/Teth" title="Teth">tes</a></i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">טש</span></span></td>
<td colspan="2">tsh</td>
<td><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">t͡ʃ</span></td>
<td>č</td>
<td><i>tes shin</i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">י</span></span></td>
<td colspan="2">y, i</td>
<td><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">j, i</span></td>
<td>j, i</td>
<td><i><a href="/wiki/Yodh" title="Yodh">yud</a></i></td>
<td>Consonantal [j] when the first character in a syllable. Vocalic [i] otherwise.</td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">יִ</span></span></td>
<td>i</td>
<td>(none)</td>
<td colspan="2"><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">i</span></td>
<td><i>khirik yud</i></td>
<td>Used only following a consonantal י or adjacent to another vowel.</td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">יי</span></span></td>
<td>ey</td>
<td>ei, ai</td>
<td><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">ɛj</span></td>
<td>ej</td>
<td><i>tsvey yudn</i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ײַ</span></span></td>
<td>ay</td>
<td>(none)</td>
<td colspan="2"><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">aj</span></td>
<td><i>pasekh tsvey yudn</i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">כּ</span></span></td>
<td colspan="4"><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">k</span></td>
<td><i><a href="/wiki/Kaph" title="Kaph">kof</a></i></td>
<td>Used only in words of Semitic origin.</td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">כ</span></span></td>
<td rowspan="2">kh</td>
<td rowspan="2">ch</td>
<td colspan="2" rowspan="2"><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">x</span></td>
<td><i>khof</i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ך</span></span></td>
<td><i>lange khof</i></td>
<td>Final form. Used only at the end of a word.</td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ל</span></span></td>
<td colspan="2">l</td>
<td><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">l, ʎ</span></td>
<td>l</td>
<td><i><a href="/wiki/Lamed" title="Lamed" class="mw-redirect">lamed</a></i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">מ</span></span></td>
<td colspan="4" rowspan="2"><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">m</span></td>
<td><i><a href="/wiki/Mem" title="Mem">mem</a></i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ם</span></span></td>
<td><i>shlos mem</i></td>
<td>Final form. Used only at the end of a word.</td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">נ</span></span></td>
<td colspan="4"><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">n</span></td>
<td><i><a href="/wiki/Nun_(letter)" title="Nun (letter)">nun</a></i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ן</span></span></td>
<td colspan="2">n, m</td>
<td><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">n</span>, <span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">ŋ</span>, <span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">m</span></td>
<td>n, m</td>
<td><i>lange nun</i></td>
<td>Final form. Used only at the end of a word.</td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ס</span></span></td>
<td colspan="4"><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">s</span></td>
<td><i><a href="/wiki/Samekh" title="Samekh">samekh</a></i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ע</span></span></td>
<td colspan="2">e</td>
<td><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">ɛ</span>, <span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">ə</span></td>
<td>e</td>
<td><i><a href="/wiki/Ayin" title="Ayin">ayin</a></i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">פּ</span></span></td>
<td colspan="4"><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">p</span></td>
<td><i><a href="/wiki/Pe_(letter)" title="Pe (letter)">pey</a></i></td>
<td>Has no separate final form.</td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">פֿ</span></span></td>
<td colspan="4"><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">f</span></td>
<td rowspan="2"><i>fey</i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">פ</span></span></td>
<td>(none)</td>
<td colspan="2">f</td>
<td>(f)</td>
<td>Non-YIVO alternative to פֿ.</td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ף</span></span></td>
<td colspan="4">f</td>
<td><i>lange fey</i></td>
<td>Final form. Used only at the end of a word.</td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">צ</span></span></td>
<td rowspan="2">ts</td>
<td rowspan="2">tz</td>
<td rowspan="2"><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">ts</span></td>
<td rowspan="2">c</td>
<td><i><a href="/wiki/Tsade" title="Tsade">tsadek</a></i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ץ</span></span></td>
<td><i>lange tsadek</i></td>
<td>Final form. Used only at the end of a word.</td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ק</span></span></td>
<td colspan="4">k</td>
<td><i><a href="/wiki/Qoph" title="Qoph">kuf</a></i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ר</span></span></td>
<td colspan="2">r</td>
<td><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">ʀ</span></td>
<td>r</td>
<td><i><a href="/wiki/Resh" title="Resh">reysh</a></i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ש</span></span></td>
<td colspan="2">sh</td>
<td><span title="Representation in the International Phonetic Alphabet (IPA)" class="IPA">ʃ</span></td>
<td>š</td>
<td><i><a href="/wiki/Shin_(letter)" title="Shin (letter)">shin</a></i></td>
<td></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">שׂ</span></span></td>
<td colspan="4">s</td>
<td><i>sin</i></td>
<td rowspan="3">Used only in words of Semitic origin.</td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">תּ</span></span></td>
<td colspan="4">t</td>
<td><i><a href="/wiki/Taw_(letter)" title="Taw (letter)" class="mw-redirect">tof</a></i></td>
</tr>
<tr>
<td style="font-size: larger"><span style="font-size:190%;"><span lang="yi" xml:lang="yi">ת</span></span></td>
<td colspan="4">s</td>
<td><i>sof</i></td>
</tr>
</table>"""


pat = re.compile(u'<span lang="yi" xml:lang="yi">(.+?)</span>.*?<td[^>]*>(.+?)</td>', re.DOTALL)
rempat = re.compile(u'</?[^>]+>')

dic = {}
for it in pat.finditer(table):
    dic[it.group(1)] = rempat.sub("", it.group(2)) 

# overwrite by human intervention ...
dic[u"י"] = 'y'

for c in tweet:
    if c in dic:
        if len(dic[c]) > 1:
            sys.stdout.write(u"(%s)" % dic[c])
        else:
            sys.stdout.write(u"%s" % dic[c])
    else:
        sys.stdout.write(c)
sys.stdout.write('\n')

