설명
bestio : ramdisk , backup/recovery

커널 버전 4.x버전으로 port 해놓았음
bio 구조체의 변수 변경 
기존
bio->bi_rw
-->
bio->bi_oft

변경된거에 맞게 코드 수정 

