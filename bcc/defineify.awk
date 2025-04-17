BEGIN                       {quoting=0;}
/^@quote/                   {quoting=1; next}
/^@end/                     {quoting=0; next}
                            {if (quoting) { printf("%-100s\\\n",$0); } else { print; } }
END                         {printf("\n\n")}