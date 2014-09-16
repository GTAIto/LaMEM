function a = FDSTAGSaveMarkersParallelMatlab(A,fname)
% This function saves model setup into a parallel configuration
%       A - structure built with ParallelMatlab_CreatePhases.m and contais:
%            W - width of domain in X-dir
%            L - length of domain in Y-dir
%            H - height of domain in Z-dir
%            nump_x - no. of particles in X-dir
%            nump_y - no. of particles in Y-dir
%            nump_z - no. of particles in Z-dir
%            Phase  - phase information of the particles
%            Temp  - Temperature information of the particles
%            x  - vector containing the x coordinates of particles
%            y  - vector containing the y coordinates of particles
%            z  - vector containing the y coordinates of particles
%            npart_x - no. of particles/cell in X-dir
%            npart_y - no. of particles/cell in Y-dir
%            npart_z - no. of particles/cell in Z-dir
%       fname - name of the file with the processor configuration;
%               in LaMEM this is saved with -SavePartitioning 1


% ----------- Function begin ----------- %
if isdir('MatlabInputParticles')
else
    mkdir MatlabInputParticles
end

% No. of properties the markers carry: x,y,z-coord, phase, T
num_prop      = 5;

% Read Processor Partitioning
[Nprocx,Nprocy,Nprocz,xc,yc,zc] = GetProcessorPartitioning(fname);
Nproc                           = Nprocx*Nprocy*Nprocz;
[num,num_i,num_j,num_k]         = get_numscheme(Nprocx,Nprocy,Nprocz);

% Grid
[X,Y,Z] = meshgrid(A.x,A.y,A.z);
X       = permute(X,[2 1 3]);
Y       = permute(Y,[2 1 3]);
Z       = permute(Z,[2 1 3]);
    
% Get particles of respective procs    
[xi,ix_start,ix_end] = get_ind(A.x,xc,Nprocx,A.nump_x);
[yi,iy_start,iy_end] = get_ind(A.y,yc,Nprocy,A.nump_y);
[zi,iz_start,iz_end] = get_ind(A.z,zc,Nprocz,A.nump_z);

x_start(num)= ix_start(num_i);
x_end(num)  = ix_end(num_i);
y_start(num)= iy_start(num_j);
y_end(num)  = iy_end(num_j);
z_start(num)= iz_start(num_k);
z_end(num)  = iz_end(num_k);

% Partition grid cells
xi_cell     = xi/A.npart_x;
yi_cell     = yi/A.npart_y;
zi_cell     = zi/A.npart_z;

cell_x(num) = xi_cell(num_i);
cell_y(num) = yi_cell(num_j);
cell_z(num) = zi_cell(num_k);

% Loop over all processors partition
for num=1:Nproc
    
    part_x   = X(x_start(num):x_end(num),y_start(num):y_end(num),z_start(num):z_end(num));
    part_y   = Y(x_start(num):x_end(num),y_start(num):y_end(num),z_start(num):z_end(num));
    part_z   = Z(x_start(num):x_end(num),y_start(num):y_end(num),z_start(num):z_end(num));
    part_phs = A.Phase(x_start(num):x_end(num),y_start(num):y_end(num),z_start(num):z_end(num));
    part_T   = A.Temp(x_start(num):x_end(num),y_start(num):y_end(num),z_start(num):z_end(num));
    
    % No. of particles per processor
    num_particles = size(part_x,1)* size(part_x,2) * size(part_x,3); 
    
    % Information vector per processor
    lvec_info(1)  = num_particles;
  
    part_x   = part_x(:);
    part_y   = part_y(:);
    part_z   = part_z(:);
    part_phs = part_phs(:);
    part_T   = part_T(:);
    
    for i=1:num_particles
        lvec_prtcls((i-1)*num_prop+ 1) = part_x(i);      %x
        lvec_prtcls((i-1)*num_prop+ 2) = part_y(i);      %y
        lvec_prtcls((i-1)*num_prop+ 3) = part_z(i);      %z
        lvec_prtcls((i-1)*num_prop+ 4) = part_phs(i);    %phase
        lvec_prtcls((i-1)*num_prop+ 5) = part_T(i);      %T
        
    end
    
    % Output files
    fname = sprintf('./MatlabInputParticles/Particles.%d.out', num-1);
    disp(['Writing file -> ',fname])
     lvec_output    = [lvec_info(:); lvec_prtcls(:)];
    
    
    PetscBinaryWrite(fname,lvec_output);
    
    %         % For debugging - Ascii output
    %         fname = sprintf('./MatlabInputParticles/Particles.ascii.%d.out', num-1);
    %         disp(['Writing file -> ',fname])
    %         fid = fopen(fname, 'w');
    %         fprintf(fid, '%d\n',lvec_info);
    %         fprintf(fid, '%d\n',lvec_prtcls);
    %         fclose(fid);
    
   
    clearvars part_x part_y part_z part_phs lvec_info lvec_prtcls id_sort No_id_vec No_id lvec_output
end

end
% ----------- Function end ----------- %

% --------------------------------------
function [Nprocx,Nprocy,Nprocz,xc,yc,zc] = GetProcessorPartitioning(test)
% Read Processor Partitioning
fid=PetscOpenFile(test);

Nprocx=read(fid,1,'int32');
Nprocy=read(fid,1,'int32');
Nprocz=read(fid,1,'int32');

nnodx=read(fid,1,'int32');
nnody=read(fid,1,'int32');
nnodz=read(fid,1,'int32');

ix=read(fid,Nprocx+1,'int32');
iy=read(fid,Nprocy+1,'int32');
iz=read(fid,Nprocz+1,'int32');

CharLength=read(fid,1,'double');

xcoor=read(fid,nnodx,'double');
ycoor=read(fid,nnody,'double');
zcoor=read(fid,nnodz,'double');

close(fid);

% Dimensionalize
xcoor = xcoor*CharLength;
ycoor = ycoor*CharLength;
zcoor = zcoor*CharLength;

ix = ix+1;
iy = iy+1;
iz = iz+1;

xc = xcoor(ix);
yc = ycoor(iy);
zc = zcoor(iz);

end

% --------------------------------------
function [n,nix,njy,nkz] = get_numscheme(Nprocx,Nprocy,Nprocz)
num=0;
for k=1:Nprocz
    for j=1:Nprocy
        for i=1:Nprocx
            num=num+1;
            n(num)   = num;
            nix(num)= i;
            njy(num)= j;
            nkz(num)= k;
        end
    end
end

end

% --------------------------------------
function [xi,ix_start,ix_end] = get_ind(x,xc,Nprocx,nump_x)
    if Nprocx == 1
        xi       = nump_x;
        ix_start = 1;
        ix_end   = nump_x;
    else
        for k= 1:Nprocx
                if k==1
                    xi(k) = length(x(x>=xc(k)& x<=xc(k+1)));
                else
                    xi(k) = length(x(x>xc(k) & x<=xc(k+1)));
                end
        end
        ix_start = cumsum([0,xi(1:end-1)])+1;
        ix_end   = cumsum(xi(1:end));
    end
end
