% function D = removeFractionSucceedingRuns(Data)
%
% Removes some fraction of succeeding runs.
% The particular succeeding runs removed are chosen randomly.
% Updates X, Indices, Findices, Sindices and records the seed used to 
% generate the pseudorandom vector of indices.
% Assumes that Data contains X, Indices, Findices, Sindices

function D = removeFractionSucceedingRuns(Data, fraction)
    D = Data;

    if fraction < 0 | fraction > 1
        error('the fraction to remove is either negative or greater than the whole');
    end;

    D.fraction = fraction;

    if fraction > 0
        indices = find(Data.Sindices);
        seed = sum(100*clock); 
        rand('state', seed);
        D.seed = seed;
        V = randperm(numel(indices))';
        remove = indices(V(1:round(fraction * numel(indices))));

        if any(Data.Findices(remove,:))
            error('these indices were selected exclusively from the succeeding run indices, so none of them should belong to failing runs');
        end

        keep = find(not(explode(remove, size(Data.X,2))));

        D.Sindices = Data.Sindices(keep,:); 
        D.Findices = Data.Findices(keep,:); 
        D.X = Data.X(:, keep');
        D.Indices = Data.Indices(keep,:);
    end
