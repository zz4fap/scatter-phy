function [output] = myFilterv2(coeffs, input)

output = zeros(length(input),1);
for i=0:1:length(input)-1
    
    for j=0:1:length(coeffs)-1
        
        input_idx = (i-j)+1;
        
        if(input_idx <= 0) 
            break;
        else
            output(i+1) = output(i+1) + coeffs(j+1)*input(input_idx);
        end
               
    end
end

end