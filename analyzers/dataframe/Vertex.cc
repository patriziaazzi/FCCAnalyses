//#include "ReconstructedParticle2Track.h"
#include "Vertex.h"

#include "TFile.h"
#include "TString.h"


#include  "MCParticle.h"

int get_nTracks( ROOT::VecOps::RVec<edm4hep::TrackState> tracks) {
   int nt = tracks.size();
   return nt;
}


// 
// Selection of particles based on the d0 / z0 significances of the associated track
//

selTracks::selTracks( float arg_d0sig_min, float arg_d0sig_max, float arg_z0sig_min, float arg_z0sig_max) : m_d0sig_min(arg_d0sig_min), 
		m_d0sig_max  ( arg_d0sig_max ), m_z0sig_min( arg_z0sig_min ), m_z0sig_max (arg_z0sig_max) { };

ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData>  selTracks::operator() ( ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> recop,
  										ROOT::VecOps::RVec<edm4hep::TrackState> tracks  ) {
  ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData>  result;
  result.reserve(recop.size());

    for (size_t i = 0; i < recop.size(); ++i) {
    auto & p = recop[i];
      if (p.tracks_begin<tracks.size()) {
	  auto & tr = tracks.at( p.tracks_begin );
          double d0sig = fabs( tr.D0 / sqrt( tr.covMatrix[0]) ) ;
          if ( fabs( d0sig ) > m_d0sig_max || fabs( d0sig ) < m_d0sig_min  ) continue;
          double z0sig = fabs( tr.Z0 / sqrt( tr.covMatrix[12]) );
          if ( fabs( z0sig ) > m_z0sig_max || fabs( z0sig ) < m_z0sig_min  ) continue;
          result.emplace_back(p);
      }
    }
  return result;
}


//
// Selection of primary particles based on the matching of RecoParticles
// to MC particles
//

ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> SelPrimaryTracks ( ROOT::VecOps::RVec<int> recind, ROOT::VecOps::RVec<int> mcind, 
				ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> reco,  ROOT::VecOps::RVec<edm4hep::MCParticleData> mc,
				TVector3 MC_EventPrimaryVertex) {

  ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> result;
  result.reserve(reco.size());

  // Event primary vertex:
  double xvtx0 = MC_EventPrimaryVertex[0];
  double yvtx0 = MC_EventPrimaryVertex[1];
  double zvtx0 = MC_EventPrimaryVertex[2];

  for (unsigned int i=0; i<recind.size();i++) {
    double xvtx = mc.at(mcind.at(i)).vertex.x ;
    double yvtx = mc.at(mcind.at(i)).vertex.y ;
    double zvtx = mc.at(mcind.at(i)).vertex.z ;
    // primary particle ?
    double zero = 1e-12;
    if ( fabs( xvtx - xvtx0) < zero && fabs( yvtx - yvtx0) < zero && fabs( zvtx -zvtx0) < zero ) {
	int reco_idx = recind.at(i); 
        result.push_back( reco.at( reco_idx )  );
    }

  }
  return result;
}



// ---------------------------------------------------------------
//
// Interface of  the vertexing code of Franco Bedeschi
// see https://www.pi.infn.it/~bedeschi/RD_FA/Software/
// VertexParam.c,    dated 2 January  2021
//
// ---------------------------------------------------------------



TVectorD get_trackParam( edm4hep::TrackState & atrack) {
    double d0 =atrack.D0 ;
    double phi0 = atrack.phi ;
    double omega = atrack.omega ;
    double z0 = atrack.Z0 ;
    double tanlambda = atrack.tanLambda ;
    TVectorD res(5);

    double scale0 = 1e-3;   //convert mm to m
    double scale1 = 1;
    double scale2 = 0.5*1e3;  // C = rho/2, convert from mm-1 to m-1
    double scale3 = 1e-3 ;  //convert mm to m
    double scale4 = 1.;

  scale2 = -scale2 ;   // sign of omega

    // Same units and definitions as Franco :
    //scale0= 1.;
    //scale2 = 1.;
    //scale3 = 1.;

    res[0] = d0 * scale0;   
    res[1] = phi0 * scale1 ;
    res[2] = omega * scale2 ;   
    res[3] = z0 * scale3 ;  
    res[4] = tanlambda * scale4 ;
    return res;
}

TMatrixDSym get_trackCov( edm4hep::TrackState &  atrack) {
    std::array<float, 15> covMatrix = atrack.covMatrix;
    TMatrixDSym covM(5);

    double scale0 = 1e-3;
    double scale1 = 1;
    double scale2 = 0.5*1e3;
    double scale3 = 1e-3 ;
    double scale4 = 1.;

    // Same units and definitions as Franco :
    //scale0= 1.;
    //scale2 = 1.;
    //scale3 = 1.;

  scale2 = -scale2 ;   // sign of omega

    covM[0][0] = covMatrix[0] *scale0 * scale0;
    covM[0][1] = covMatrix[1] *scale0 * scale1;
    covM[0][2] = covMatrix[2] *scale0 * scale2;
    covM[0][3] = covMatrix[3] *scale0 * scale3;
    covM[0][4] = covMatrix[4] *scale0 * scale4;

    covM[1][0] = covM[0][1];
    covM[2][0] = covM[0][2] ;
    covM[3][0] = covM[0][3] ;
    covM[4][0] = covM[0][4];

    covM[1][1] = covMatrix[5] *scale1 * scale1;
    covM[1][2] = covMatrix[6] *scale1 * scale2;
    covM[1][3] = covMatrix[7] *scale1 * scale3;
    covM[1][4] = covMatrix[8] *scale1 * scale4;;

    covM[2][1] = covM[1][2] ;
    covM[3][1] = covM[1][3];
    covM[4][1] = covM[1][4];

    covM[2][2] = covMatrix[9] *scale2 *scale2;
    covM[2][3] = covMatrix[10] * scale2 * scale3;
    covM[2][4] = covMatrix[11] * scale2 * scale4;

    covM[3][2] = covM[2][3];
    covM[4][2] = covM[2][4];

    covM[3][3] = covMatrix[12] * scale3 * scale3;
    covM[3][4] = covMatrix[13] * scale3 * scale4;

    covM[4][3] = covM[3][4];

    covM[4][4] = covMatrix[14] * scale4*scale4;;

    return covM;
}





TMatrixDSym RegInv3(TMatrixDSym &Smat0)
{
	//
	// Regularized inversion of symmetric 3x3 matrix with positive diagonal elements
	//
	TMatrixDSym Smat = Smat0;
	Int_t N = Smat.GetNrows();
	if (N != 3)
	{
		std::cout << "RegInv3 called with  matrix size != 3. Abort & return standard inversion." << std::endl;
		return Smat.Invert();
	}
	TMatrixDSym D(N); D.Zero();
	Bool_t dZero = kTRUE;	// No elements less or equal 0 on the diagonal
	for (Int_t i = 0; i < N; i++) if (Smat(i, i) <= 0.0)dZero = kFALSE;
	if (dZero)
	{
		for (Int_t i = 0; i < N; i++) D(i, i) = 1.0 / TMath::Sqrt(Smat(i, i));
		TMatrixDSym RegMat = Smat.Similarity(D);
		TMatrixDSym Q(2);
		for (Int_t i = 0; i < 2; i++)
		{
			for (Int_t j = 0; j < 2; j++)Q(i, j) = RegMat(i, j);
		}
		Double_t Det = 1 - Q(0, 1)*Q(1, 0);
		TMatrixDSym H(2);
		H = Q;
		H(0, 1) = -Q(0, 1);
		H(1, 0) = -Q(1, 0);
		TVectorD p(2);
		p(0) = RegMat(0, 2);
		p(1) = RegMat(1, 2);
		Double_t pHp = H.Similarity(p);
		Double_t h = pHp-Det;
		//
		TMatrixDSym pp(2); pp.Rank1Update(p);
		TMatrixDSym F = (h*H) - pp.Similarity(H);
		F *= 1.0 / Det;
		TVectorD b = H*p;
		TMatrixDSym InvReg(3);
		for (Int_t i = 0; i < 2; i++)
		{
			InvReg(i, 2) = b(i);
			InvReg(2, i) = b(i);
			for (Int_t j = 0; j < 2; j++) InvReg(i, j) = F(i, j);
		}
		InvReg(2, 2) = -Det;
		//
		InvReg *= 1.0 / h;
		//
		return InvReg.Similarity(D);
	}
	else
	{
		//cout << "RegInv3: found negative elements in diagonal" << endl;
		return Smat.Invert();
	}
}
//
TMatrixDSym RegInv2(TMatrixDSym &Smat0)
{
	//
	TMatrixDSym Smat = Smat0; 
	Int_t N = Smat.GetNrows();
	if (N != 2)
	{
		std::cout << "RegInv2 called with  matrix size != 2. Abort & return standard inversion." << std::endl;
		return Smat.Invert();
	}
	TMatrixDSym RegOut(N);
	TMatrixDSym D(N); D.Zero();
	Bool_t dZero = kTRUE;	// No elements less or equal 0 on the diagonal
	for (Int_t i = 0; i < N; i++) if (Smat(i, i) <= 0.0)dZero = kFALSE;
	if (dZero)
	{
		for (Int_t i = 0; i < N; i++) D(i, i) = 1.0 / TMath::Sqrt(Smat(i, i));
		TMatrixDSym RegMat = Smat.Similarity(D);
		Double_t Det = 1.0-RegMat(0,1)*RegMat(1,0);
		RegMat(0, 1) *= -1;
		RegMat(1, 0) *= -1;
		RegMat *= 1.0 / Det;
		//
		RegOut = RegMat.Similarity(D);
	}
	else
	{
		RegOut = Smat.Invert();
		//cout << "RegInv2: found negative elements in diagonal." << endl;
	}
	//
	return RegOut;
}
//

TVector3 VertexFitter0( ROOT::VecOps::RVec<edm4hep::TrackState> tracks )

{
	//
	// Preliminary estimate of the vertex position
	// based on transformation of track into points
	// and vertices into lines
	// No steering of track parameters
	// No error calculation
	//

        int Ntr = tracks.size();
        TVector3 dummy(-1e12,-1e12,-1e12);
        if (Ntr <= 0) return dummy;


	TVectorD xv(3);		// returned vertex position
	//
	TMatrixDSym H(2);
	TVectorD xvt(2);
	TVectorD cxy(2);
	//
	// Loop on tracks for transverse fit
	//
	TVectorD x0(2); x0.Zero();
	Double_t Rv = 0.0;	    // Radius of first iteration
	Int_t Ntry = 0;
	Int_t TryMax = 10;
	Double_t epsi = 1000.;		// Starting stability
	Double_t eps = 0.0001;		// vertex stability required
	while (epsi > eps && Ntry < TryMax)
	{
		H.Zero(); cxy.Zero();
		for (Int_t i = 0; i < Ntr; i++)
		{
			// Get track helix parameters and their covariance matrix 
			//ObsTrk *t = tracks[i];
			edm4hep::TrackState t = tracks[i] ;
			//TVectorD par = t->GetObsPar();
			TVectorD par = get_trackParam( t ) ;
			//TMatrixDSym C = t->GetCov();
			TMatrixDSym C = get_trackCov( t) ;
			//
			// Transverse fit
			Double_t D0i = par(0);
			Double_t phi = par(1);
			Double_t Ci = par(2);
			Double_t Di = (D0i*(1. + Ci*D0i) + Rv*Rv*Ci) / (1. + 2 * Ci*D0i);
			Double_t sDi2 = C(0, 0);
			//
			TVectorD ni(2);
			ni(0) = -TMath::Sin(phi);
			ni(1) = TMath::Cos(phi);
			TMatrixDSym Hadd(2);
			Hadd.Rank1Update(ni, 1);		// Tensor product of vector ni with itself
			H += (1.0 / sDi2)*Hadd;
			cxy += (Di / sDi2)*ni;

		}
		//
		TMatrixDSym Cov = RegInv2(H);
		xvt = Cov*cxy;
		xv.SetSub(0, xvt);	// Store x,y of vertex
		Rv = TMath::Sqrt(xv(0)*xv(0) + xv(1)*xv(1));
		TVectorD dx = xvt - x0;
		epsi = H.Similarity(dx);
		x0 = xvt;
		Ntry++;
		//cout << "Vtx0: Iteration #" << Ntry << ", eps = " << epsi << ", x = " << xv(0) << ", y = " << xv(1);
	}
	//
	// Longitudinal fit
	Double_t Rv2 = Rv*Rv;
	//
	// Loop on tracks for longitudinal fit
	Double_t hz = 0.0;
	Double_t cz = 0.0;
	for (Int_t i = 0; i < Ntr; i++)
	{
		// Get track helix parameters and their covariance matrix 
		//ObsTrk *t = tracks[i];
		//TVectorD par = t->GetObsPar();
		//TMatrixDSym C = t->GetCov();
		edm4hep::TrackState t = tracks[i];
		TVectorD par = get_trackParam( t ) ;
		TMatrixDSym C = get_trackCov( t) ;
		//
		// Longitudinal fit
		Double_t zi = par(3);
		Double_t cti = par(4);
		Double_t Di = par(0);
		Double_t Ci = par(2);
		Double_t sZi2 = C(3, 3);
		//
		hz += 1 / sZi2;
		Double_t arg = TMath::Sqrt(TMath::Max(0.0, Rv*Rv - Di*Di) / (1. + 2 * Ci*Di));
		cz += (cti*arg + zi) / sZi2;
	}
	xv(2) = cz / hz;
	//cout << ", z = " << xv(2) << endl;
	//
	///return xv;

        TVector3 result( xv(0)*1e3, xv(1)*1e3, xv(2)*1e3 );  // convert to mm
        return result;

}
//
TMatrixD Fill_A(TVectorD par, Double_t phi)
{
	TMatrixD A(3, 5);
	//
	// Decode input arrays
	//
	Double_t D = par(0);
	Double_t p0 = par(1);
	Double_t C = par(2);
	Double_t z0 = par(3);
	Double_t ct = par(4);
	//
	// Fill derivative matrix dx/d alpha
	// D
	A(0, 0) = -TMath::Sin(p0);
	A(1, 0) = TMath::Cos(p0);
	A(2, 0) = 0.0;
	// phi0
	A(0, 1) = -D*TMath::Cos(p0) + (TMath::Cos(phi + p0) - TMath::Cos(p0)) / (2 * C);
	A(1, 1) = -D*TMath::Sin(p0) + (TMath::Sin(phi + p0) - TMath::Sin(p0)) / (2 * C);
	A(2, 1) = 0.0;
	// C
	A(0, 2) = -(TMath::Sin(phi + p0) - TMath::Sin(p0)) / (2 * C*C);
	A(1, 2) = (TMath::Cos(phi + p0) - TMath::Cos(p0)) / (2 * C*C);
	A(2, 2) = -ct*phi / (2 * C*C);
	// z0
	A(0, 3) = 0.0;
	A(1, 3) = 0.0;
	A(2, 3) = 1.0;
	// ct = lambda
	A(0, 4) = 0.0;
	A(1, 4) = 0.0;
	A(2, 4) = phi / (2 * C);
	//
	return A;
}
//
TVectorD Fill_a(TVectorD par, Double_t phi)
{
	TVectorD a(3);
	//
	// Decode input arrays
	//
	Double_t D = par(0);
	Double_t p0 = par(1);
	Double_t C = par(2);
	Double_t z0 = par(3);
	Double_t ct = par(4);
	//
	a(0) = TMath::Cos(phi + p0) / (2 * C);
	a(1) = TMath::Sin(phi + p0) / (2 * C);
	a(2) = ct / (2 * C);
	//
	return a;
}
//
TVectorD Fill_x0(TVectorD par)
{
	TVectorD x0(3);
	//
	// Decode input arrays
	//
	Double_t D = par(0);
	Double_t p0 = par(1);
	Double_t C = par(2);
	Double_t z0 = par(3);
	Double_t ct = par(4);
	//
	x0(0) = -D *TMath::Sin(p0);
	x0(1) = D*TMath::Cos(p0);
	x0(2) = z0;
	//
	return x0;
}
//
TVectorD Fill_x(TVectorD par, Double_t phi)
{
	TVectorD x(3);
	//
	// Decode input arrays
	//
	Double_t D = par(0);
	Double_t p0 = par(1);
	Double_t C = par(2);
	Double_t z0 = par(3);
	Double_t ct = par(4);
	//
	TVectorD x0 = Fill_x0(par);
	x(0) = x0(0) + (TMath::Sin(phi + p0) - TMath::Sin(p0)) / (2 * C);
	x(1) = x0(1) - (TMath::Cos(phi + p0) - TMath::Cos(p0)) / (2 * C);
	x(2) = x0(2) + ct*phi / (2 * C);
	//
	return x;
}
//

//Double_t VertexP(Int_t Ntr, ObsTrk **tracks, TVectorD &x, TMatrixDSym &covX)

edm4hep::VertexData  VertexFitter( int Primary, ROOT::VecOps::RVec<edm4hep::ReconstructedParticleData> recoparticles,
                                        ROOT::VecOps::RVec<edm4hep::TrackState> thetracks )

{

        edm4hep::VertexData result;

        ROOT::VecOps::RVec<edm4hep::TrackState> tracks = getRP2TRK( recoparticles, thetracks );
        int Ntr = tracks.size();
        if ( Ntr <= 0) return result;

	Bool_t Debug = kFALSE;
	//
	// Get approximate vertex evaluation
	//
	//TVectorD x0 = Vertex0(Ntr, tracks);

        TVector3 ini_vtx = VertexFitter0( tracks) ;
        TVectorD x0(3);	   // convert back from mm to meters below :
        x0[0] = ini_vtx[0] *1e-3 ;
        x0[1] = ini_vtx[1] *1e-3 ;
        x0[2] = ini_vtx[2] *1e-3 ;

        TVectorD x(3);
        TMatrixDSym covX(3);

	//cout << "Preliminary vertex" << endl; x0.Print();
	//
	// Stored quantities
	Double_t *fi = new Double_t[Ntr];		// Phases 
	TVectorD **x0i = new TVectorD*[Ntr];		// Track expansion point
	TVectorD **ai = new TVectorD*[Ntr];		// dx/dphi
	Double_t *a2i = new Double_t[Ntr];		// a'Wa
	TMatrixDSym **Di = new TMatrixDSym*[Ntr];		// W-WBW
	TMatrixDSym **Wi = new TMatrixDSym*[Ntr];	// (ACA')^-1
	TMatrixDSym **Winvi = new TMatrixDSym*[Ntr];	// ACA'
	//
	// Loop on tracks to calculate everything
	//
	Int_t Ntry = 0;
	Int_t TryMax = 100;
	Double_t eps = 1.0e-9; // vertex stability
	Double_t epsi = 1000.;
	Double_t Chi2;
	//
	while (epsi > eps && Ntry < TryMax)		// Iterate until found vertex is stable
	{
		Double_t x0mod = TMath::Sqrt(x0(0)*x0(0) + x0(1)*x0(1));
		if (x0mod > 2.0) x0.Zero();	// Reset to 0 if abnormal value
		x = x0;
		TVectorD cterm(3); TMatrixDSym H(3); TMatrixDSym DW1D(3);
		covX.Zero();		// Reset vertex covariance
		cterm.Zero();	// Reset constant term
		H.Zero();		// Reset H matrix
		DW1D.Zero();
		// vertex radius approximation
		Double_t R = TMath::Sqrt(x(0)*x(0) + x(1)*x(1));
		// 
		for (Int_t i = 0; i < Ntr; i++)
		{
			// Get track helix parameters and their covariance matrix 
			//ObsTrk *t = tracks[i];
			//TVectorD par = t->GetObsPar();
			//TMatrixDSym Cov = t->GetCov(); 
			edm4hep::TrackState t = tracks[i] ;
 			TVectorD par = get_trackParam( t ) ;
			TMatrixDSym Cov = get_trackCov( t) ;
			Double_t fs;
			if (Ntry <= 0)
			{
				Double_t D = par(0);
				Double_t C = par(2);
				Double_t arg = TMath::Max(1.0e-6, (R*R - D*D) / (1 + 2 * C*D));
				fs = 2 * TMath::ASin(C*TMath::Sqrt(arg));
				fi[i] = fs;
			}
			//
			// Starting values
			//
			//Starting helix positions
			fs = fi[i];
			TVectorD xs = Fill_x(par, fs);
			x0i[i] = new TVectorD(xs);	// Starting helix position
			// W matrix = (A*C*A')^-1; W^-1 = A*C*A'
			TMatrixD A = Fill_A(par, fs);
			TMatrixDSym Winv = Cov.Similarity(A);
			//cout << "Track " << i << ", W^-1:" << endl; Winv.Print();
			Winvi[i] = new TMatrixDSym(Winv);	// Store matrix
			TMatrixDSym W = RegInv3(Winv);
			//cout << "Track " << i << ", W:" << endl; W.Print();
			Wi[i] = new TMatrixDSym(W);			// Store matrix
			// B matrices
			TVectorD a = Fill_a(par, fs);
			ai[i] = new TVectorD(a);				// Store vector
			//cout << "Track " << i << ", a vector:" << endl; a.Print();
			Double_t a2 = W.Similarity(a);
			//cout << "Track " << i << ", a2:"<<a2 << endl;
			a2i[i] = a2;		// Store
			TMatrixDSym B(3); 
			B.Rank1Update(a, 1.0);
			B *= -1. / a2;
			B.Similarity(W);
			//cout << "Track " << i << ", B matrix:" << endl; B.Print();
			// D matrices
			//cout << "Track " << i << ", before Ds calculation" << endl;
			//cout << "Track " << i << ", Wd matrix:" << endl; Wd.Print();
			TMatrixDSym Ds = W+B;
			//cout << "Track " << i << ", Ds:" << endl; Ds.Print();
			Di[i] = new TMatrixDSym(Ds);		// Store matrix
			TMatrixDSym DsW1Ds = Winv.Similarity(Ds);
			DW1D += DsW1Ds;
			// Update hessian
			H += Ds;
			// update constant term
			cterm += Ds * xs;
		}				// End loop on tracks
		//
		// update vertex position
		TMatrixDSym H1 = RegInv3(H);
		x = H1*cterm;
		// Update vertex covariance
		covX = DW1D.Similarity(H1);
		// Update phases and chi^2
		Chi2 = 0.0;
		for (Int_t i = 0; i < Ntr; i++)
		{
			TVectorD lambda = (*Di[i])*(*x0i[i] - x);
			TMatrixDSym Wm1 = *Winvi[i];
			Chi2 += Wm1.Similarity(lambda);
			TVectorD a = *ai[i];
			TVectorD b = (*Wi[i])*(x - *x0i[i]);
			for (Int_t j = 0; j < 3; j++)fi[i] += a(j)*b(j) / a2i[i];
		}
		//
		TVectorD dx = x - x0;
		x0 = x;
		// update vertex stability
		TMatrixDSym Hess = RegInv3(covX);
		epsi = Hess.Similarity(dx);
		Ntry++;
		//if (epsi >10)
		//cout << "Vtx:  Iteration #"<<Ntry<<", eps = "<<epsi<<", x = " << x(0) << ", " << x(1) << ", " << x(2) << endl;


        //
        // Cleanup
        //
	        for (Int_t i = 0; i < Ntr; i++)
        	{
                     x0i[i]->Clear();
                     Winvi[i]->Clear();
                     Wi[i]->Clear();
                     ai[i]->Clear();
                     Di[i]->Clear();
	
                     delete x0i[i];
                     delete Winvi[i];
                     delete Wi[i] ;
                     delete ai[i] ;
                     delete Di[i];
                }
	}
	//
	// Cleanup
	//
		//delete[] fi;		// Phases 
		//delete[] x0i;		// Track expansion point
		//delete[] ai;		// dx/dphi
		//delete[] a2i;		// a'Wa
		//delete [] Di;		// W-WBW
		//delete [] Wi;	// (ACA')^-1
		//delete [] Winvi;	// ACA'

                delete fi;            // Phases 
                delete x0i;           // Track expansion point
                delete ai;            // dx/dphi
                delete a2i;           // a'Wa
                delete  Di;           // W-WBW
                delete  Wi;   // (ACA')^-1
                delete  Winvi;        // ACA'

	//
	//return Chi2;

	// store the results in an edm4hep::VertexData object
	// go back from meters to millimeters for the units 
	float conv = 1e3;
        std::array<float,6> covMatrix;
        covMatrix[0] = covX(0,0) * pow(conv,2);
        covMatrix[1] = covX(0,1) * pow(conv,2);
        covMatrix[2] = covX(0,2) * pow(conv,2);
        covMatrix[3] = covX(1,1) * pow(conv,2);
        covMatrix[4] = covX(1,2) * pow(conv,2);
        covMatrix[5] = covX(2,2) * pow(conv,2);

        float Ndof = 2.0 * Ntr - 3.0; ;

        result.primary = Primary;
        result.chi2 = Chi2 /Ndof ;      // I store the normalised chi2 here
        result.position = edm4hep::Vector3f( x(0)*conv, x(1)*conv, x(2)*conv ) ;  // store the  vertex in mm
        result.covMatrix = covMatrix;
        result.algorithmType = 1;

        // Need to fill the associations ...

        return result;


}


////////////////////////////////////////////////////

