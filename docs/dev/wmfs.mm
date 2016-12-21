<map version="0.8.0">
<!-- To view this file, download free mind mapping software FreeMind from http://freemind.sourceforge.net -->
<node CREATED="1268599815107" ID="Freemind_Link_1711697288" MODIFIED="1268599915528" TEXT="WMFS - Window Manager Filesystem">
<node COLOR="#990000" CREATED="1268601195705" ID="_" MODIFIED="1268606087660" POSITION="right" TEXT="Current Work">
<node CREATED="1328207378824" ID="Freemind_Link_1136726139" MODIFIED="1328207393425" TEXT="process update events for global wm properties"/>
<node CREATED="1328207394075" ID="Freemind_Link_879980262" MODIFIED="1328207400223" TEXT="make writing to global wm properties possible"/>
<node CREATED="1328207404851" ID="Freemind_Link_427895554" MODIFIED="1328207412406" TEXT="make sendRequest more flexible and put it into XWindow"/>
</node>
<node CREATED="1288385281465" ID="Freemind_Link_915710075" MODIFIED="1288385282601" POSITION="right" TEXT="Bugs">
<node CREATED="1327859623176" ID="Freemind_Link_1745874170" MODIFIED="1327859635348" TEXT="with --xsync we somehow end up in an infinite loop"/>
</node>
<node CREATED="1268601230053" ID="Freemind_Link_870411819" MODIFIED="1268606080251" POSITION="right" TEXT="Features">
<node CREATED="1268602662030" ID="Freemind_Link_62954421" MODIFIED="1268606106748" TEXT="Windows list by">
<cloud/>
<node CREATED="1268602680023" ID="Freemind_Link_1457102476" MODIFIED="1288126052138" TEXT="Desktop"/>
<node CREATED="1268602672198" ID="Freemind_Link_1994549482" MODIFIED="1268602674786" TEXT="ID"/>
<node CREATED="1268602753936" ID="Freemind_Link_366374062" MODIFIED="1268602756948" TEXT="Client"/>
<node CREATED="1327859605011" ID="Freemind_Link_733554744" MODIFIED="1327859606367" TEXT="Head"/>
</node>
<node CREATED="1268602761745" ID="Freemind_Link_1392431811" MODIFIED="1268602776372" TEXT="window operations">
<node CREATED="1268602778353" ID="Freemind_Link_1568284932" MODIFIED="1268602796205" TEXT="move">
<node CREATED="1268680938313" ID="Freemind_Link_1456783704" MODIFIED="1272142357498" TEXT="allow move via move() system call"/>
</node>
<node CREATED="1268602798473" ID="Freemind_Link_589139076" MODIFIED="1268602799461" TEXT="resize"/>
<node CREATED="1268602800177" ID="Freemind_Link_164623261" MODIFIED="1268602803141" TEXT="warp"/>
<node CREATED="1268602805257" ID="Freemind_Link_231156667" MODIFIED="1268602829789" TEXT="layer change"/>
</node>
<node CREATED="1268603930595" ID="Freemind_Link_715305571" MODIFIED="1268603950143" TEXT="events">
<node CREATED="1268603952675" ID="Freemind_Link_186573541" MODIFIED="1268603957815" TEXT="wait for window exit">
<node CREATED="1270994992681" ID="Freemind_Link_1488904938" MODIFIED="1270994995069" TEXT="inotify!"/>
<node CREATED="1328202714930" ID="Freemind_Link_1250619109" MODIFIED="1328202721061" TEXT="or better a file that allows blocking"/>
</node>
<node CREATED="1268603958419" ID="Freemind_Link_791140729" MODIFIED="1269800244093" TEXT="wait for window creation">
<icon BUILTIN="messagebox_warning"/>
<node CREATED="1270994985370" ID="Freemind_Link_10619916" MODIFIED="1270994988167" TEXT="inotify!"/>
<node CREATED="1328202700092" ID="Freemind_Link_70332042" MODIFIED="1328202713047" TEXT="or better a file that allows to block on it, is easier"/>
</node>
<node CREATED="1268603961915" ID="Freemind_Link_1357132863" MODIFIED="1268603978000" TEXT="wait for window changes (size, position etc.)">
<node CREATED="1270995020633" ID="Freemind_Link_680689993" MODIFIED="1270995022861" TEXT="inotify!"/>
<node CREATED="1272142223382" ID="Freemind_Link_1611104487" MODIFIED="1272142238953" TEXT="Is that really sensible? Can we catch all changes this way?"/>
</node>
</node>
<node CREATED="1268603990476" ID="Freemind_Link_1683115786" MODIFIED="1268603992896" TEXT="properties">
<node CREATED="1268603994060" ID="Freemind_Link_86137218" MODIFIED="1268604001064" TEXT="make properties available"/>
<node CREATED="1268604001716" ID="Freemind_Link_516476485" MODIFIED="1268604010961" TEXT="make it possible to add new properties"/>
<node CREATED="1268604011493" ID="Freemind_Link_1882929190" MODIFIED="1268604031662" TEXT="make it possible to add new types?">
<icon BUILTIN="help"/>
</node>
</node>
</node>
<node CREATED="1268601245973" ID="Freemind_Link_1987637537" MODIFIED="1268601256483" POSITION="right" TEXT="Improvement">
<node CREATED="1268604043397" ID="Freemind_Link_907618982" MODIFIED="1268606089796" TEXT="Properties">
<cloud/>
<node CREATED="1268604058158" ID="Freemind_Link_1067221591" MODIFIED="1268604068882" TEXT="Correct modelling of X11 C API in C++ with templates for setting and getting"/>
<node CREATED="1268604167441" ID="Freemind_Link_156519194" MODIFIED="1268604177653" TEXT="Attach Atoms and Property handling to XWindow type"/>
</node>
<node CREATED="1268604942334" ID="Freemind_Link_1875819701" MODIFIED="1268604961321" TEXT="EWMH-Support-Property should be interpreted to prevent senseless lookups"/>
<node CREATED="1268604970870" ID="Freemind_Link_1405391167" MODIFIED="1268606097972" TEXT="Error Handling">
<cloud/>
<node CREATED="1268604974102" ID="Freemind_Link_726849212" MODIFIED="1272142389145" TEXT="In case of uncritical errors/warnings"/>
<node CREATED="1269800103653" ID="Freemind_Link_258010024" MODIFIED="1269800112868" TEXT="Tracing/Logging infrastructure">
<node CREATED="1288542811546" ID="Freemind_Link_1053362462" MODIFIED="1288542818510" TEXT="different logs available as files inFS"/>
</node>
<node CREATED="1288640131192" ID="Freemind_Link_1197930730" MODIFIED="1328207460740" TEXT="Maybe make exception a stream? problems with NonCopyable"/>
</node>
<node CREATED="1268605015349" ID="Freemind_Link_734241127" MODIFIED="1268605032473" TEXT="Determine how to correctly handle unicode strings">
<node CREATED="1268605045581" ID="Freemind_Link_399674431" MODIFIED="1278851015181" TEXT="interpet active locale by calling setlocale() and using w[cs]* functions"/>
</node>
<node CREATED="1268605079157" ID="Freemind_Link_1995762942" MODIFIED="1268605083529" TEXT="Avoid copying of string data"/>
<node CREATED="1268605878318" ID="Freemind_Link_1433032975" MODIFIED="1268605885146" TEXT="Also handle WM global properties/features"/>
<node CREATED="1288643510746" ID="Freemind_Link_399702302" MODIFIED="1288643518413" TEXT="Have different types of objects for WM specific adjustments">
<node CREATED="1288643519058" ID="Freemind_Link_1047000782" MODIFIED="1288643529997" TEXT="needs a creation strategy for certain objects that are specialized per WM"/>
<node CREATED="1288643530522" ID="Freemind_Link_1271915563" MODIFIED="1288643541301" TEXT="upon startup WM is detected and creation strategy adjusted accordingly"/>
</node>
</node>
<node CREATED="1288643546810" ID="Freemind_Link_103103783" MODIFIED="1288643547837" POSITION="right" TEXT="Testing">
<node CREATED="1288643548394" ID="Freemind_Link_913494045" MODIFIED="1288643563270" TEXT="Need some kind of automated testing to check whether things are still working as expected after refactoring stuff"/>
</node>
<node CREATED="1268601260973" ID="Freemind_Link_640504426" MODIFIED="1268601268163" POSITION="right" TEXT="Sources">
<node CREATED="1268601433054" ID="Freemind_Link_1569223391" LINK="http://standards.freedesktop.org/wm-spec/wm-spec-latest.html" MODIFIED="1268606738016" TEXT="EWMH spec: http://standards.freedesktop.org/wm-spec/wm-spec-latest.html"/>
<node CREATED="1268601522245" ID="Freemind_Link_1474498666" LINK="http://www.pps.jussieu.fr/~jch/software/UTF8_STRING/UTF8_STRING.text" MODIFIED="1268606765376" TEXT="UTF8_STRING type: http://www.pps.jussieu.fr/~jch/software/UTF8_STRING/UTF8_STRING.text"/>
<node CREATED="1268601539541" ID="Freemind_Link_929432812" LINK="http://www.sbin.org/doc/Xlib/chapt_12.html" MODIFIED="1268606771520" TEXT="X11 API: http://www.sbin.org/doc/Xlib/chapt_12.html"/>
</node>
<node CREATED="1287955916315" ID="Freemind_Link_695529608" MODIFIED="1287955921455" POSITION="right" TEXT="Structure">
<node CREATED="1287955922139" ID="Freemind_Link_584705886" MODIFIED="1287955924287" TEXT="X11">
<node CREATED="1287955924883" ID="Freemind_Link_447463840" MODIFIED="1287955933895" TEXT="Any X11 only API &amp; implementation"/>
</node>
<node CREATED="1287955936155" ID="Freemind_Link_1290199254" MODIFIED="1287955937639" TEXT="Fuse">
<node CREATED="1287955938307" ID="Freemind_Link_592112481" MODIFIED="1287955945399" TEXT="Any FUSE only API &amp; implementation"/>
</node>
<node CREATED="1287955946083" ID="Freemind_Link_824684947" MODIFIED="1287955947415" TEXT="WMFS">
<node CREATED="1287955947995" ID="Freemind_Link_1790144290" MODIFIED="1287955970735" TEXT="Connection between X11 and Fuse -&gt; adds the actual WMFS logic by facilitating that functionatliy"/>
</node>
<node CREATED="1287955971619" ID="Freemind_Link_1367276195" MODIFIED="1287955975103" TEXT="Common">
<node CREATED="1287955975603" ID="Freemind_Link_1230427782" MODIFIED="1287955989415" TEXT="Generic functionality independent of X11 and FUSE and WMFS. Only POSIX and Linux"/>
</node>
</node>
</node>
</map>
