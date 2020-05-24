function fibonacci(n)  
    begin
        if n = 0 then
            begin
                return 0;
            end;
        if n = 1 then
            begin
                 return 1;
            end;
        return fibonacci(n -1) + fibonacci(n - 2); 
    end
var i;
begin
    i := 0;
    while i < 10 do  
        begin
            write fibonacci(i);
            writeln;
            i := i + 1;
        end    
end.